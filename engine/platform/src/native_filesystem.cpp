#include <dream/platform/native_filesystem.hpp>
#if defined(__unix__) || defined(__APPLE__) || defined(__ANDROID__)
#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <limits>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
namespace dream::platform {
namespace {
struct Fd {
    int value{-1};
    explicit Fd(int fd=-1) noexcept:value(fd){}
    ~Fd(){ if(value>=0) (void)::close(value); }
    Fd(const Fd&)=delete;
    Fd& operator=(const Fd&)=delete;
    Fd(Fd&& other) noexcept:value(other.value){other.value=-1;}
    Fd& operator=(Fd&& other) noexcept {
        if(this!=&other){if(value>=0)(void)::close(value);value=other.value;other.value=-1;}
        return *this;
    }
};
FileError from_errno(int e) noexcept {
    switch(e){
    case 0:return FileError::None;
    case ENOENT:return FileError::NotFound;
    case EEXIST:return FileError::AlreadyExists;
    case EACCES: case EPERM: case ELOOP:return FileError::PermissionDenied;
    case ENOTDIR:return FileError::NotDirectory;
    case EISDIR:return FileError::IsDirectory;
    case ENAMETOOLONG: case EINVAL:return FileError::InvalidPath;
    case ENOSPC: case EROFS: case EIO:return FileError::Io;
    default:return FileError::Io;
    }
}
bool split(std::string_view path,std::vector<std::string>& parts){
    if(!is_valid_virtual_path(path))return false;
    std::vector<std::string> next;
    std::size_t begin=0;
    while(begin<path.size()){
        const auto end=path.find('/',begin);
        next.emplace_back(path.substr(begin,end==std::string_view::npos?end:end-begin));
        if(end==std::string_view::npos)break;
        begin=end+1;
    }
    parts.swap(next);
    return !parts.empty();
}
bool open_dirs(int root,const std::vector<std::string>& parts,
               std::size_t count,Fd& result,FileError& error){
    Fd current(::dup(root));
    if(current.value<0){error=from_errno(errno);return false;}
    for(std::size_t i=0;i<count;++i){
        const int fd=::openat(current.value,parts[i].c_str(),
            O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW);
        if(fd<0){error=from_errno(errno);return false;}
        current=Fd(fd);
    }
    result=std::move(current);error=FileError::None;return true;
}
bool parent_for(int root,const std::vector<std::string>& parts,
                Fd& parent,const std::string*& leaf,FileError& error){
    if(parts.empty()){error=FileError::InvalidPath;return false;}
    if(!open_dirs(root,parts,parts.size()-1,parent,error))return false;
    leaf=&parts.back();return true;
}
bool supported(const struct stat& st) noexcept{
    return S_ISDIR(st.st_mode)||S_ISREG(st.st_mode);
}
std::string temp_name(){
    static std::atomic<std::uint64_t> seq{0};
    return ".dream-write-"+std::to_string(static_cast<unsigned long long>(::getpid()))+
        "-"+std::to_string(seq.fetch_add(1,std::memory_order_relaxed));
}
}
NativeFileSystem::NativeFileSystem(std::string root){
    if(root.empty()||root.find('\0')!=std::string::npos){
        initialization_error_="Filesystem root must be a non-empty native path.";return;
    }
    root_fd_=::open(root.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW);
    if(root_fd_<0)initialization_error_="Cannot open filesystem root: "+std::string(std::strerror(errno));
}
NativeFileSystem::~NativeFileSystem(){if(root_fd_>=0)(void)::close(root_fd_);}
bool NativeFileSystem::valid()const noexcept{return root_fd_>=0;}
const std::string& NativeFileSystem::initialization_error()const noexcept{return initialization_error_;}

bool NativeFileSystem::read_file(std::string_view path,std::vector<std::byte>& output,
    std::size_t max_bytes,FileError& error)const{
    if(!valid()){error=FileError::Io;return false;}
    std::vector<std::string> parts;
    if(!split(path,parts)){error=FileError::InvalidPath;return false;}
    Fd parent;const std::string* leaf=nullptr;
    if(!parent_for(root_fd_,parts,parent,leaf,error))return false;
    Fd file(::openat(parent.value,leaf->c_str(),O_RDONLY|O_CLOEXEC|O_NOFOLLOW));
    if(file.value<0){error=from_errno(errno);return false;}
    struct stat st{};
    if(::fstat(file.value,&st)!=0){error=from_errno(errno);return false;}
    if(!S_ISREG(st.st_mode)){error=S_ISDIR(st.st_mode)?FileError::IsDirectory:FileError::Unsupported;return false;}
    if(st.st_size<0||static_cast<std::uint64_t>(st.st_size)>max_bytes){error=FileError::TooLarge;return false;}
    try{
        std::vector<std::byte> temp;
        const auto initial=static_cast<std::uint64_t>(st.st_size);
        if(initial>temp.max_size()){error=FileError::TooLarge;return false;}
        temp.reserve(static_cast<std::size_t>(initial));
        std::array<std::byte,65536> buffer{};
        for(;;){
            const auto n=::read(file.value,buffer.data(),buffer.size());
            if(n<0){if(errno==EINTR)continue;error=from_errno(errno);return false;}
            if(n==0)break;
            const auto amount=static_cast<std::size_t>(n);
            if(temp.size()>max_bytes||amount>max_bytes-temp.size()){error=FileError::TooLarge;return false;}
            temp.insert(temp.end(),buffer.begin(),buffer.begin()+static_cast<std::ptrdiff_t>(amount));
        }
        output.swap(temp);error=FileError::None;return true;
    }catch(...){error=FileError::Io;return false;}
}
bool NativeFileSystem::write_file(std::string_view path,std::span<const std::byte> data,FileError& error){
    if(!valid()){error=FileError::Io;return false;}
    std::vector<std::string> parts;
    if(!split(path,parts)){error=FileError::InvalidPath;return false;}
    Fd parent;const std::string* leaf=nullptr;
    if(!parent_for(root_fd_,parts,parent,leaf,error))return false;
    Fd file;std::string temp;
    for(unsigned attempt=0;attempt<16;++attempt){
        temp=temp_name();
        const int fd=::openat(parent.value,temp.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC|O_NOFOLLOW,0600);
        if(fd>=0){file=Fd(fd);break;}
        if(errno!=EEXIST){error=from_errno(errno);return false;}
    }
    if(file.value<0){error=FileError::AlreadyExists;return false;}
    const auto cleanup=[&](){(void)::unlinkat(parent.value,temp.c_str(),0);};
    std::size_t written=0;
    while(written<data.size()){
        const auto chunk=std::min(data.size()-written,static_cast<std::size_t>(std::numeric_limits<ssize_t>::max()));
        const auto n=::write(file.value,data.data()+written,chunk);
        if(n<0){if(errno==EINTR)continue;error=from_errno(errno);cleanup();return false;}
        if(n==0){error=FileError::Io;cleanup();return false;}
        written+=static_cast<std::size_t>(n);
    }
    if(::fsync(file.value)!=0){error=from_errno(errno);cleanup();return false;}
    file=Fd{};
    if(::renameat(parent.value,temp.c_str(),parent.value,leaf->c_str())!=0){
        error=from_errno(errno);cleanup();return false;
    }
    error=FileError::None;return true;
}
bool NativeFileSystem::exists(std::string_view path,bool& result,FileError& error)const{
    if(!valid()){error=FileError::Io;return false;}
    std::vector<std::string> parts;
    if(!split(path,parts)){error=FileError::InvalidPath;return false;}
    Fd parent;const std::string* leaf=nullptr;
    if(!parent_for(root_fd_,parts,parent,leaf,error))return false;
    struct stat st{};
    if(::fstatat(parent.value,leaf->c_str(),&st,AT_SYMLINK_NOFOLLOW)!=0){
        if(errno==ENOENT){result=false;error=FileError::None;return true;}
        error=from_errno(errno);return false;
    }
    if(!supported(st)){error=FileError::PermissionDenied;return false;}
    result=true;error=FileError::None;return true;
}
bool NativeFileSystem::create_directories(std::string_view path,FileError& error){
    if(!valid()){error=FileError::Io;return false;}
    std::vector<std::string> parts;
    if(!split(path,parts)){error=FileError::InvalidPath;return false;}
    Fd current(::dup(root_fd_));
    if(current.value<0){error=from_errno(errno);return false;}
    for(const auto& part:parts){
        if(::mkdirat(current.value,part.c_str(),0700)!=0&&errno!=EEXIST){error=from_errno(errno);return false;}
        const int fd=::openat(current.value,part.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW);
        if(fd<0){error=from_errno(errno);return false;}
        current=Fd(fd);
    }
    error=FileError::None;return true;
}
bool NativeFileSystem::remove_file(std::string_view path,FileError& error){
    if(!valid()){error=FileError::Io;return false;}
    std::vector<std::string> parts;
    if(!split(path,parts)){error=FileError::InvalidPath;return false;}
    Fd parent;const std::string* leaf=nullptr;
    if(!parent_for(root_fd_,parts,parent,leaf,error))return false;
    if(::unlinkat(parent.value,leaf->c_str(),0)!=0){error=from_errno(errno);return false;}
    error=FileError::None;return true;
}
bool NativeFileSystem::list_directory(std::string_view path,
    std::vector<DirectoryEntry>& output,FileError& error)const{
    if(!valid()){error=FileError::Io;return false;}
    Fd directory;
    if(path.empty()){
        directory=Fd(::dup(root_fd_));
        if(directory.value<0){error=from_errno(errno);return false;}
    }else{
        std::vector<std::string> parts;
        if(!split(path,parts)){error=FileError::InvalidPath;return false;}
        if(!open_dirs(root_fd_,parts,parts.size(),directory,error))return false;
    }
    const int copy=::dup(directory.value);
    if(copy<0){error=from_errno(errno);return false;}
    DIR* dir=::fdopendir(copy);
    if(!dir){const int saved=errno;(void)::close(copy);error=from_errno(saved);return false;}
    try{
        std::vector<DirectoryEntry> entries;
        for(;;){
            errno=0;const auto* item=::readdir(dir);
            if(!item){const int saved=errno;(void)::closedir(dir);if(saved){error=from_errno(saved);return false;}break;}
            const std::string_view name(item->d_name);
            if(name=="."||name==".."||!is_valid_virtual_path(name))continue;
            struct stat st{};
            if(::fstatat(::dirfd(dir),item->d_name,&st,AT_SYMLINK_NOFOLLOW)!=0){
                if(errno==ENOENT)continue;
                const int saved=errno;(void)::closedir(dir);error=from_errno(saved);return false;
            }
            if(!supported(st))continue;
            entries.push_back({std::string(name),{
                static_cast<std::uint64_t>(st.st_size<0?0:st.st_size),
                S_ISDIR(st.st_mode),S_ISREG(st.st_mode)}});
        }
        std::sort(entries.begin(),entries.end(),[](const auto& a,const auto& b){return a.name<b.name;});
        output.swap(entries);error=FileError::None;return true;
    }catch(...){(void)::closedir(dir);error=FileError::Io;return false;}
}
} // namespace dream::platform
#endif
