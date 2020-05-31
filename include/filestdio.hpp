#ifndef FILESTDIO_HPP
#define FILESTDIO_HPP

#include <string>
#include <system_error>
#if defined(_WIN32)
#  pragma push_macro("WIN32_LEAN_AND_MEAN")
#  pragma push_macro("NOMINMAX")
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <Windows.h>
#  pragma pop_macro("WIN32_LEAN_AND_MEAN")
#  pragma pop_macro("NOMINMAX")
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
        Redirect(const std::string& filename, Stream s):
            originalFile(s), stream(s),
            file(filename, (stream == Stream::in) ? File::Mode::read :
                 (stream == Stream::out || stream == Stream::err) ? File::Mode::write :
                 File::Mode::none)
        {
#if defined(_WIN32)
            if (!SetStdHandle(getStdHandle(stream), file))
                throw std::system_error(GetLastError(), std::system_category(), "Failed to set std handle");
#else
            const int streamFileDescriptor = getStreamFileDescriptor(stream);
            int result = dup2(file, streamFileDescriptor);
            while (result == -1 && errno == EINTR)
                result = dup2(file, streamFileDescriptor);

            if (result == -1)
                throw std::system_error(errno, std::system_category(), "Failed to duplicate file descriptor");
#endif
        }

        ~Redirect()
        {
#if defined(_WIN32)
             if (!SetStdHandle(getStdHandle(stream), originalFile))
                 throw std::system_error(GetLastError(), std::system_category(), "Failed to set std handle");
#else
            const int streamFileDescriptor = getStreamFileDescriptor(stream);
            while (dup2(originalFile, streamFileDescriptor) == -1 && errno == EINTR);
#endif
        }

        Redirect(const Redirect&) = delete;
        Redirect& operator=(const Redirect&) = delete;

    private:
#if defined(_WIN32)
        static constexpr DWORD getStdHandle(Stream stream) noexcept
        {
            return (stream == Stream::in) ? STD_INPUT_HANDLE :
                (stream == Stream::out) ? STD_OUTPUT_HANDLE :
                (stream == Stream::err) ? STD_ERROR_HANDLE :
                -1;
        }
#else
        static constexpr int getStreamFileDescriptor(Stream stream) noexcept
        {
            return (stream == Stream::in) ? STDIN_FILENO :
                (stream == Stream::out) ? STDOUT_FILENO :
                (stream == Stream::err) ? STDERR_FILENO :
                -1;
        }
#endif

        class StdFile final
        {
        public:
            StdFile(Stream stream):
#if defined(_WIN32)
                handle(GetStdHandle(getStdHandle(stream))
#else
                fileDescriptor(dup(getStreamFileDescriptor(stream)))
#endif
            {
#if defined(_WIN32)
                if (handle == INVALID_HANDLE_VALUE)
                    throw std::system_error(GetLastError(), std::system_category(), "Failed to get standard device handle");
#else
                while (fileDescriptor == -1 && errno == EINTR)
                    fileDescriptor = dup(getStreamFileDescriptor(stream));

                if (fileDescriptor == -1)
                    throw std::system_error(errno, std::system_category(), "Failed to duplicate file descriptor");
#endif
            }

            ~StdFile()
            {
#if defined(_WIN32)
#else
                if (fileDescriptor != -1) close(fileDescriptor);
#endif
            }

            StdFile(const StdFile&) = delete;
            StdFile& operator=(const StdFile&) = delete;

#if defined(_WIN32)
            operator HANDLE() const noexcept { return handle; }
#else
            operator int() const noexcept { return fileDescriptor; }
#endif
        private:
#if defined(_WIN32)
            HANDLE handle = INVALID_HANDLE_VALUE;
#else
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
#if defined(_WIN32)
                DWORD access = (mode == Mode::read) ? GENERIC_READ :
                    (mode == Mode::write) ? GENERIC_WRITE : -1;

                DWORD creationDisposition = (mode == Mode::read) ? OPEN_EXISTING :
                    (mode == Mode::write) ? OPEN_ALWAYS : -1;

                handle = CreateFileA(filename.c_str(), access, 0,
                                     nullptr, creationDisposition,
                                     FILE_ATTRIBUTE_NORMAL, nullptr);

                if (handle == INVALID_HANDLE_VALUE)
                    throw std::system_error(GetLastError(), std::system_category(), "Failed to open file");
#else
                const int flags = (mode == Mode::read) ? O_RDONLY :
                    (mode == Mode::write) ? (O_CREAT | O_WRONLY) : -1;
                fileDescriptor = open(filename.c_str(), flags, S_IRUSR | S_IWUSR);
                while (fileDescriptor == -1 && errno == EINTR)
                    fileDescriptor = open(filename.c_str(), flags);

                if (fileDescriptor == -1)
                    throw std::system_error(errno, std::system_category(), "Failed to open file");
#endif
            }

            ~File()
            {
#if defined(_WIN32)
                if (handle != INVALID_HANDLE_VALUE) CloseHandle(handle);
#else
                if (fileDescriptor != -1) close(fileDescriptor);
#endif
            }

            File(const File&) = delete;
            File& operator=(const File&) = delete;

#if defined(_WIN32)
            operator HANDLE() const noexcept { return handle; }
#else
            operator int() const noexcept { return fileDescriptor; }
#endif
        private:
#if defined(_WIN32)
            HANDLE handle = INVALID_HANDLE_VALUE;
#else
            int fileDescriptor = -1;
#endif
        };

        StdFile originalFile;
        Stream stream;
        File file;
    };
}

#endif // FILESTDIO_HPP
