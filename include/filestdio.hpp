#ifndef FILESTDIO_HPP
#define FILESTDIO_HPP

#include <string>
#include <system_error>
#if defined(_WIN32)
#else
#  include <fcntl.h>
#  include <unistd.h>
#endif

namespace filestdio
{
    enum class Stream
    {
        in,
        out,
        err
    };

    class Redirect final
    {
    public:
        Redirect(const std::string& filename, Stream stream):
            originalFile(stream)
        {
            const File::Mode mode = (stream == Stream::in) ? File::Mode::read :
                (stream == Stream::out || stream == Stream::err) ? File::Mode::write :
                File::Mode::none;
            File file(filename, mode);
#if defined(_WIN32)
#else
            int streamFileDescriptor = (stream == Stream::in) ? STDIN_FILENO :
                (stream == Stream::out) ? STDOUT_FILENO :
                (stream == Stream::err) ? STDERR_FILENO :
                -1;

            int result = dup2(file, streamFileDescriptor);
            while (result == -1 && errno == EINTR)
                result = dup2(file, streamFileDescriptor);

            if (result == -1)
                throw std::system_error(errno, std::system_category(), "Failed to duplicate file descriptor");
#endif
        }

    private:
        class StdFile final
        {
        public:
#if defined(_WIN32)
            StdFile(Stream stream)
            {
            }
#else
            StdFile(Stream stream):
                streamFileDescriptor((stream == Stream::in) ? STDIN_FILENO :
                                     (stream == Stream::out) ? STDOUT_FILENO :
                                     (stream == Stream::err) ? STDERR_FILENO :
                                     -1)
            {
                fileDescriptor = dup(streamFileDescriptor);
                while (fileDescriptor == -1 && errno == EINTR)
                    fileDescriptor = dup(streamFileDescriptor);

                if (fileDescriptor == -1)
                    throw std::system_error(errno, std::system_category(), "Failed to duplicate file descriptor");
            }
#endif

            ~StdFile()
            {
#if defined(_WIN32)
#else
                if (fileDescriptor != -1)
                {
                    dup2(fileDescriptor, streamFileDescriptor);
                    close(fileDescriptor);
                }
#endif
            }

            StdFile(const StdFile&) = delete;
            StdFile& operator=(const StdFile&) = delete;

        private:
#if defined(_WIN32)
#else
            int streamFileDescriptor = -1;
            int fileDescriptor = -1;
#endif
        };

        class File final
        {
        public:
            enum class Mode
            {
                none,
                read,
                write
            };

            File(const std::string& filename, Mode mode)
            {
                const int flags = (mode == Mode::read) ? O_RDONLY :
                    (mode == Mode::write) ? O_WRONLY : -1;
                fileDescriptor = open(filename.c_str(), flags);
                while (fileDescriptor == -1 && errno == EINTR)
                    fileDescriptor = open(filename.c_str(), flags);

                if (fileDescriptor == -1)
                    throw std::system_error(errno, std::system_category(), "Failed to open file");
            }

            ~File()
            {
                if (fileDescriptor != -1) close(fileDescriptor);
            }

            File(const File&) = delete;
            File& operator=(const File&) = delete;

            operator int() const noexcept { return fileDescriptor; }
        private:
#if defined(_WIN32)
#else
            int fileDescriptor = -1;
#endif
        };

        StdFile originalFile;
    };
}

#endif // FILESTDIO_HPP
