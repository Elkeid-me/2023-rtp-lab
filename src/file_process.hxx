#ifndef FILE_PROCESS_H
#define FILE_PROCESS_H

namespace file_process
{
    void close(int fd);

    // 为了保证文件描述符能正确关闭而创造的包装类.
    // 如果文件描述符仅在一个函数内使用，可以使用这个类本身；
    // 否则，建议用 std::shared_ptr 包一层.
    class fd_wrapper
    {
    private:
        int m_file_descriptor{-1};

    public:
        fd_wrapper &operator=(const fd_wrapper &) = delete;
        fd_wrapper(const fd_wrapper &) = delete;

        fd_wrapper(int);
        ~fd_wrapper();

        void open(int fd);

        bool is_valid() const;
        int get_file_descriptor();
    };
}

#endif
