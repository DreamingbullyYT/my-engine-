#include <dream/platform/native_filesystem.hpp>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>
#if defined(__unix__) || defined(__APPLE__) || defined(__ANDROID__)
#include <unistd.h>
namespace {
class TempDir final {
public:
    TempDir(){
        auto pattern=(std::filesystem::temp_directory_path()/"dream-fs-XXXXXX").string();
        std::vector<char> chars(pattern.begin(),pattern.end());chars.push_back('\0');
        const auto* path=::mkdtemp(chars.data());assert(path!=nullptr);path_=path;
    }
    ~TempDir(){std::error_code ignored;std::filesystem::remove_all(path_,ignored);}
    const std::filesystem::path& path()const noexcept{return path_;}
private:std::filesystem::path path_;
};
std::vector<std::byte> bytes(std::string_view text){
    std::vector<std::byte> out;out.reserve(text.size());
    for(const unsigned char c:text)out.push_back(static_cast<std::byte>(c));
    return out;
}
std::string text(const std::vector<std::byte>& data){
    std::string out;out.reserve(data.size());
    for(const auto value:data)out.push_back(static_cast<char>(std::to_integer<unsigned char>(value)));
    return out;
}
}
int main(){
    TempDir root;TempDir outside;
    dream::platform::NativeFileSystem fs(root.path().string());
    assert(fs.valid());
    using dream::platform::FileError;
    FileError error=FileError::Io;
    assert(fs.create_directories("save/world/region",error));
    const auto content=bytes("native filesystem");
    assert(fs.write_file("save/world/region/data.bin",content,error));
    std::vector<std::byte> output{std::byte{0x42}};
    assert(fs.read_file("save/world/region/data.bin",output,128,error));
    assert(text(output)=="native filesystem");
    const auto previous=output;
    assert(!fs.read_file("save/world/region/data.bin",output,3,error));
    assert(error==FileError::TooLarge&&output==previous);
    assert(!fs.read_file("../outside",output,128,error)&&error==FileError::InvalidPath);
    assert(!fs.write_file("/absolute",content,error)&&error==FileError::InvalidPath);
    bool exists=false;
    assert(fs.exists("save/world/region/data.bin",exists,error)&&exists);
    assert(fs.exists("save/world/region/missing",exists,error)&&!exists);
    std::vector<dream::platform::DirectoryEntry> entries;
    assert(fs.list_directory("save/world/region",entries,error));
    assert(entries.size()==1&&entries[0].name=="data.bin"&&entries[0].info.is_regular_file);
    const auto external=outside.path()/"secret.txt";
    {std::ofstream file(external);file<<"outside";}
    const auto dir_link=root.path()/"escape";
    assert(::symlink(outside.path().c_str(),dir_link.c_str())==0);
    assert(!fs.read_file("escape/secret.txt",output,128,error));
    const auto file_link=root.path()/"linked.txt";
    assert(::symlink(external.c_str(),file_link.c_str())==0);
    assert(!fs.read_file("linked.txt",output,128,error));
    assert(fs.remove_file("linked.txt",error));
    entries.clear();
    assert(fs.list_directory("",entries,error));
    assert(entries.size()==1&&entries[0].name=="save"&&entries[0].info.is_directory);
    entries.clear();
    assert(!fs.list_directory(".",entries,error)&&error==FileError::InvalidPath);
    assert(fs.remove_file("save/world/region/data.bin",error));
    assert(fs.exists("save/world/region/data.bin",exists,error)&&!exists);
    return 0;
}
#else
int main(){return 0;}
#endif
