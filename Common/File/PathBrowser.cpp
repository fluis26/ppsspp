#include <algorithm>
#include <cstring>
#include <set>

#include "ppsspp_config.h"

#include "Common/Net/HTTPClient.h"
#include "Common/Net/URL.h"

#include "Common/File/PathBrowser.h"
#include "Common/File/FileUtil.h"
#include "Common/StringUtils.h"
#include "Common/TimeUtil.h"
#include "Common/Log.h"
#include "Common/Thread/ThreadUtil.h"

#include "Core/System.h"

bool LoadRemoteFileList(const std::string &url, bool *cancel, std::vector<File::FileInfo> &files) {
	http::Client http;
	Buffer result;
	int code = 500;
	std::vector<std::string> responseHeaders;

	http.SetUserAgent(StringFromFormat("PPSSPP/%s", PPSSPP_GIT_VERSION));

	Url baseURL(url);
	if (!baseURL.Valid()) {
		return false;
	}

	// Start by requesting the list of files from the server.
	if (http.Resolve(baseURL.Host().c_str(), baseURL.Port())) {
		if (http.Connect(2, 20.0, cancel)) {
			http::RequestProgress progress(cancel);
			code = http.GET(baseURL.Resource().c_str(), &result, responseHeaders, &progress);
			http.Disconnect();
		}
	}

	if (code != 200 || (cancel && *cancel)) {
		return false;
	}

	std::string listing;
	std::vector<std::string> items;
	result.TakeAll(&listing);

	std::string contentType;
	for (const std::string &header : responseHeaders) {
		if (startsWithNoCase(header, "Content-Type:")) {
			contentType = header.substr(strlen("Content-Type:"));
			// Strip any whitespace (TODO: maybe move this to stringutil?)
			contentType.erase(0, contentType.find_first_not_of(" \t\r\n"));
			contentType.erase(contentType.find_last_not_of(" \t\r\n") + 1);
		}
	}

	// TODO: Technically, "TExt/hTml    ; chaRSet    =    Utf8" should pass, but "text/htmlese" should not.
	// But unlikely that'll be an issue.
	bool parseHtml = startsWithNoCase(contentType, "text/html");
	bool parseText = startsWithNoCase(contentType, "text/plain");

	if (parseText) {
		// Plain text format - easy.
		SplitString(listing, '\n', items);
	} else if (parseHtml) {
		// Try to extract from an automatic webserver directory listing...
		GetQuotedStrings(listing, items);
	} else {
		ERROR_LOG(IO, "Unsupported Content-Type: %s", contentType.c_str());
		return false;
	}

	for (std::string item : items) {
		// Apply some workarounds.
		if (item.empty())
			continue;
		if (item.back() == '\r')
			item.pop_back();
		if (item == baseURL.Resource())
			continue;

		File::FileInfo info;
		info.name = item;
		info.fullName = baseURL.Relative(item).ToString();
		info.isDirectory = endsWith(item, "/");
		info.exists = true;
		info.size = 0;
		info.isWritable = false;

		files.push_back(info);
	}

	return !files.empty();
}

std::vector<File::FileInfo> ApplyFilter(std::vector<File::FileInfo> files, const char *filter) {
	std::set<std::string> filters;
	if (filter) {
		std::string tmp;
		while (*filter) {
			if (*filter == ':') {
				filters.insert(std::move(tmp));
			} else {
				tmp.push_back(*filter);
			}
			filter++;
		}
		if (!tmp.empty())
			filters.insert(std::move(tmp));
	}

	auto pred = [&](const File::FileInfo &info) {
		if (info.isDirectory || !filter)
			return false;
		std::string ext = File::GetFileExtension(info.fullName);
		return filters.find(ext) == filters.end();
	};
	files.erase(std::remove_if(files.begin(), files.end(), pred), files.end());
	return files;
}

PathBrowser::~PathBrowser() {
	std::unique_lock<std::mutex> guard(pendingLock_);
	pendingCancel_ = true;
	pendingStop_ = true;
	pendingCond_.notify_all();
	guard.unlock();

	if (pendingThread_.joinable()) {
		pendingThread_.join();
	}
}

// Normalize slashes.
void PathBrowser::SetPath(const std::string &path) {
	if (path[0] == '!') {
		path_ = path;
		HandlePath();
		return;
	}
	path_ = path;
	for (size_t i = 0; i < path_.size(); i++) {
		if (path_[i] == '\\') path_[i] = '/';
	}
	if (!path_.size() || (path_[path_.size() - 1] != '/'))
		path_ += "/";
	HandlePath();
}

void PathBrowser::HandlePath() {
	if (!path_.empty() && path_[0] == '!') {
		if (pendingActive_)
			ResetPending();
		ready_ = true;
		return;
	}
	if (!startsWith(path_, "http://") && !startsWith(path_, "https://")) {
		if (pendingActive_)
			ResetPending();
		ready_ = true;
		return;
	}

	std::lock_guard<std::mutex> guard(pendingLock_);
	ready_ = false;
	pendingActive_ = true;
	pendingCancel_ = false;
	pendingFiles_.clear();
	pendingPath_ = path_;
	pendingCond_.notify_all();

	if (pendingThread_.joinable())
		return;

	pendingThread_ = std::thread([&] {
		SetCurrentThreadName("PathBrowser");

		std::unique_lock<std::mutex> guard(pendingLock_);
		std::vector<File::FileInfo> results;
		std::string lastPath;
		while (!pendingStop_) {
			while (lastPath == pendingPath_ && !pendingCancel_) {
				pendingCond_.wait(guard);
			}
			lastPath = pendingPath_;
			bool success = false;
			if (!lastPath.empty()) {
				guard.unlock();
				results.clear();
				success = LoadRemoteFileList(lastPath, &pendingCancel_, results);
				guard.lock();
			}

			if (pendingPath_ == lastPath) {
				if (success && !pendingCancel_) {
					pendingFiles_ = results;
				}
				pendingPath_.clear();
				lastPath.clear();
				ready_ = true;
			}
		}
	});
}

void PathBrowser::ResetPending() {
	std::lock_guard<std::mutex> guard(pendingLock_);
	pendingCancel_ = true;
	pendingPath_.clear();
}

bool PathBrowser::IsListingReady() {
	return ready_;
}

std::string PathBrowser::GetFriendlyPath() const {
	std::string str = GetPath();
	// Show relative to memstick root if there.
	std::string root = GetSysDirectory(DIRECTORY_MEMSTICK_ROOT);
	for (size_t i = 0; i < root.size(); i++) {
		if (root[i] == '\\')
			root[i] = '/';
	}

	if (startsWith(str, root)) {
		return std::string("ms:/") + str.substr(root.size());
	}

#if PPSSPP_PLATFORM(LINUX) || PPSSPP_PLATFORM(MAC)
	char *home = getenv("HOME");
	if (home != nullptr && !strncmp(str.c_str(), home, strlen(home))) {
		str = std::string("~") + str.substr(strlen(home));
	}
#endif
	return str;
}

bool PathBrowser::GetListing(std::vector<File::FileInfo> &fileInfo, const char *filter, bool *cancel) {
	std::unique_lock<std::mutex> guard(pendingLock_);
	while (!IsListingReady() && (!cancel || !*cancel)) {
		// In case cancel changes, just sleep.
		guard.unlock();
		sleep_ms(100);
		guard.lock();
	}

#ifdef _WIN32
	if (path_ == "/") {
		// Special path that means root of file system.
		std::vector<std::string> drives = File::GetWindowsDrives();
		for (auto drive = drives.begin(); drive != drives.end(); ++drive) {
			if (*drive == "A:/" || *drive == "B:/")
				continue;
			File::FileInfo fake;
			fake.fullName = *drive;
			fake.name = *drive;
			fake.isDirectory = true;
			fake.exists = true;
			fake.size = 0;
			fake.isWritable = false;
			fileInfo.push_back(fake);
		}
	}
#endif

	if (startsWith(path_, "http://") || startsWith(path_, "https://")) {
		fileInfo = ApplyFilter(pendingFiles_, filter);
		return true;
	} else {
		File::GetFilesInDir(path_.c_str(), &fileInfo, filter);
		return true;
	}
}

bool PathBrowser::CanNavigateUp() {
/* Leaving this commented out, not sure if there's a use in UWP for navigating up from the user data folder.
#if PPSSPP_PLATFORM(UWP)
	// Can't navigate up from memstick folder :(
	if (path_ == GetSysDirectory(DIRECTORY_MEMSTICK_ROOT)) {
		return false;
	}
#endif
*/
	if (path_ == "/" || path_ == "") {
		return false;
	}
	return true;
}

void PathBrowser::NavigateUp() {
	// Upwards.
	// Check for windows drives.
	if (path_.size() == 3 && path_[1] == ':') {
		path_ = "/";
	} else if (startsWith(path_, "http://") || startsWith(path_, "https://")) {
		// You can actually pin "remote disc streaming" (which I didn't even realize until recently).
		// This prevents you from getting the path browser into very weird states:
		path_ = "/";
		// It's ok to just go directly to root without more checking since remote disc streaming
		// does not yet support folders.
	} else {
		size_t slash = path_.rfind('/', path_.size() - 2);
		if (slash != std::string::npos)
			path_ = path_.substr(0, slash + 1);
	}
}

// TODO: Support paths like "../../hello"
void PathBrowser::Navigate(const std::string &path) {
	if (path == ".")
		return;
	if (path == "..") {
		NavigateUp();
	} else {
		if (path.size() > 2 && path[1] == ':' && path_ == "/")
			path_ = path;
		else
			path_ = path_ + path;
		if (path_[path_.size() - 1] != '/')
			path_ += "/";
	}
	HandlePath();
}
