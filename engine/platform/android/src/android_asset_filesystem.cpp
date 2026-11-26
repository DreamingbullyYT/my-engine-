#include <dream/platform/android/android_asset_filesystem.hpp>
#if defined(__ANDROID__)
#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>
#include <android/asset_manager.h>
namespace dream::platform {
namespace {
class Asset final {
public:
    explicit Asset(AAsset* p=nullptr) noexcept:p_(p){}
    ~Asset(){if(p_)AAsset_close(p_);}
    Asset(const Asset&)=delete;
    Asset& operator=(const Asset&)=delete;
    Asset(Asset&& o) noexcept:p_(o.p_){o.p_=nullptr;}
    Asset& operator=(Asset&& o) noexcept {
        if(this!=&o){if(p_)AAsset_close(p_);p_=o.p_;o.p_=nullptr;}return *this;
    }
    AAsset* get()const noexcept{return p_;}
    explicit operator bool()const noexcept{return p_!=nullptr;}
private:AAsset* p_{};
};
class AssetDir final {
public:
    explicit AssetDir(AAssetDir* p=nullptr)noexcept:p_(p){}
    ~AssetDir(){if(p_)AAssetDir_close(p_);}
    AssetDir(const AssetDir&)=delete;
    AssetDir& operator=(const AssetDir&)=delete;
    AAssetDir* get()const noexcept{return p_;}
    explicit operator bool()const noexcept{return p_!=nullptr;}
private:AAssetDir* p_{};
};
bool valid_path(std::string_view p,bool root,FileError& e)noexcept{
    if(p.empty()&&root){e=FileError::None;return true;}
    if(!is_valid_virtual_path(p)){e=FileError::InvalidPath;return false;}
    e=FileError::None;return true;
}
bool child_name(std::string_view dir,std::string_view name,std::string& out){
    if(name.empty()||!is_valid_virtual_path(name))return false;
    if(dir.empty()){out.assign(name);return true;}
    out.reserve(dir.size()+1U+name.size());
    out.assign(dir);out.push_back('/');out.append(name);
    return is_valid_virtual_path(out);
}
}
bool AndroidAssetFileSystem::read_file(std::string_view path,
    std::vector<std::byte>& output,std::size_t max_bytes,FileError& error)const{
    if(!valid()){error=FileError::Io;return false;}
    if(!valid_path(path,false,error))return false;
    const std::string name(path);
    Asset asset(AAssetManager_open(manager_,name.c_str(),AASSET_MODE_STREAMING));
    if(!asset){error=FileError::NotFound;return false;}
    const off64_t length=AAsset_getLength64(asset.get());
    if(length<0||static_cast<std::uint64_t>(length)>max_bytes){
        error=FileError::TooLarge;return false;
    }
    try{
        std::vector<std::byte> temp;
        const auto expected=static_cast<std::uint64_t>(length);
        if(expected>temp.max_size()){error=FileError::TooLarge;return false;}
        temp.reserve(static_cast<std::size_t>(expected));
        std::array<std::byte,64U*1024U> buffer{};
        for(;;){
            const int count=AAsset_read(asset.get(),buffer.data(),buffer.size());
            if(count<0){error=FileError::Io;return false;}
            if(count==0)break;
            const auto amount=static_cast<std::size_t>(count);
            if(temp.size()>max_bytes||amount>max_bytes-temp.size()){
                error=FileError::TooLarge;return false;
            }
            temp.insert(temp.end(),buffer.begin(),
                buffer.begin()+static_cast<std::ptrdiff_t>(amount));
        }
        if(temp.size()!=static_cast<std::size_t>(expected)){
            error=FileError::Io;return false;
        }
        output.swap(temp);error=FileError::None;return true;
    }catch(...){error=FileError::Io;return false;}
}
bool AndroidAssetFileSystem::write_file(std::string_view path,
    std::span<const std::byte>,FileError& error){
    if(!valid()){error=FileError::Io;return false;}
    if(!valid_path(path,false,error))return false;
    error=FileError::Unsupported;return false;
}
bool AndroidAssetFileSystem::exists(std::string_view path,bool& result,
    FileError& error)const{
    if(!valid()){error=FileError::Io;return false;}
    if(!valid_path(path,false,error))return false;
    const std::string name(path);
    Asset asset(AAssetManager_open(manager_,name.c_str(),AASSET_MODE_STREAMING));
    result=static_cast<bool>(asset);error=FileError::None;return true;
}
bool AndroidAssetFileSystem::create_directories(std::string_view path,FileError& error){
    if(!valid()){error=FileError::Io;return false;}
    if(!valid_path(path,false,error))return false;
    error=FileError::Unsupported;return false;
}
bool AndroidAssetFileSystem::remove_file(std::string_view path,FileError& error){
    if(!valid()){error=FileError::Io;return false;}
    if(!valid_path(path,false,error))return false;
    error=FileError::Unsupported;return false;
}
bool AndroidAssetFileSystem::list_directory(std::string_view path,
    std::vector<DirectoryEntry>& output,FileError& error)const{
    if(!valid()){error=FileError::Io;return false;}
    if(!valid_path(path,true,error))return false;
    const std::string directory_name(path);
    AssetDir directory(AAssetManager_openDir(manager_,directory_name.c_str()));
    if(!directory){error=FileError::NotFound;return false;}
    try{
        std::vector<DirectoryEntry> entries;
        while(const char* item=AAssetDir_getNextFileName(directory.get())){
            const std::string_view name(item);
            std::string full_name;
            if(!child_name(path,name,full_name))continue;
            Asset asset(AAssetManager_open(manager_,full_name.c_str(),AASSET_MODE_STREAMING));
            if(!asset)continue; // APK directory objects are not exposed as files.
            const off64_t length=AAsset_getLength64(asset.get());
            if(length<0){error=FileError::Io;return false;}
            entries.push_back({std::string(name),
                FileInfo{static_cast<std::uint64_t>(length),false,true}});
        }
        std::sort(entries.begin(),entries.end(),
            [](const auto& a,const auto& b){return a.name<b.name;});
        output.swap(entries);error=FileError::None;return true;
    }catch(...){error=FileError::Io;return false;}
}
} // namespace dream::platform
#endif
