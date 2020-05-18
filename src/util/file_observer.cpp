#include "pch.hpp"
#include "jaws/util/file_observer.hpp"
#include "file_observer_impl.hpp"

namespace jaws::util {

FileObserver::FileObserver()
{
    _impl = std::make_unique<detail::FileObserverImpl>();
}

FileObserver::~FileObserver() = default;


FileObserver::Handle FileObserver::AddObservedFile(const std::filesystem::path& path)
{
    return _impl->AddObservedFile(path);
}


bool FileObserver::RemoveObservedFile(jaws::util::FileObserver::Handle handle)
{
    return _impl->RemoveObservedFile(handle);
}


std::vector<FileObserver::Result> FileObserver::Poll()
{
    return _impl->Poll();
}


} // namespace jaws::util
