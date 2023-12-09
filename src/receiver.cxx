#include "rtp_header.hxx"
#include "tools.hxx"

int main(int argc, char **argv)
{
    if (argc != 5)
        logs::error("参数错误. 你可以这样使用: ", argv[0],
                    " [receiver ip] [receiver port] [file path] [window size] [mode]");

    logs::debug("Receiver: 正在退出");
    return 0;
}
