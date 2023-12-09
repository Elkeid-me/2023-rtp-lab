#include "file_process.hxx"
#include "error_process.hxx"
#include <cstdio>
#include <unistd.h>

namespace file_process
{
    static void close(int fd)
    {
        if (::close(fd) < 0)
            error_process::unix_error("`file_process::close()' error: ");
    }

    file_descriptor_wrapper::~file_descriptor_wrapper()
    {
        if (m_file_descriptor >= 0)
            ::file_process::close(m_file_descriptor);

        m_file_descriptor = -1;
    }

    file_descriptor_wrapper::file_descriptor_wrapper(int file_descriptor)
        : m_file_descriptor{file_descriptor}
    {
    }

    bool file_descriptor_wrapper::is_valid() const { return m_file_descriptor >= 0; }
    int file_descriptor_wrapper::get_file_descriptor() { return m_file_descriptor; }

    std_FILE_wrapper::std_FILE_wrapper(std::FILE *file_ptr): m_file_ptr{file_ptr} {}
    std_FILE_wrapper::~std_FILE_wrapper()
    {
        if (m_file_ptr != nullptr)
            std::fclose(m_file_ptr);

        m_file_ptr = nullptr;
    }

    bool std_FILE_wrapper::is_valid() const { return m_file_ptr != nullptr; }
    std::FILE *std_FILE_wrapper::get_file_ptr() { return m_file_ptr; }
};
