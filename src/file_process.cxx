#include "file_process.hxx"
#include "error_process.hxx"
#include "tools.hxx"
#include <unistd.h>

namespace file_process
{
    void close(int fd)
    {
        if (::close(fd) < 0)
            error_process::unix_error("`close()` 错误: ");
    }

    fd_wrapper::~fd_wrapper()
    {
        log_debug("文件描述符包装器 `", m_file_descriptor, "` 析构");
        if (m_file_descriptor >= 0)
            ::file_process::close(m_file_descriptor);

        m_file_descriptor = -1;
    }

    void fd_wrapper::open(int fd)
    {
        if (m_file_descriptor >= 0)
            ::file_process::close(m_file_descriptor);

        m_file_descriptor = fd;
    }

    fd_wrapper::fd_wrapper(int file_descriptor) : m_file_descriptor{file_descriptor}
    {
        log_debug("文件描述符包装器 `", file_descriptor, "` 构造");
    }

    bool fd_wrapper::is_valid() const { return m_file_descriptor >= 0; }
    int fd_wrapper::get_file_descriptor() { return m_file_descriptor; }
}
