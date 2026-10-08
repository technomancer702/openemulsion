// SPDX-License-Identifier: MPL-2.0

#include <windows.h>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "ColorSpaceConfig.h"
#include "FilmModules.h"

extern bool RunOpenEmulsionOpenCL(void*, int, int, double, const float*, const float*, float*);

namespace {
void require(bool condition) { if (!condition) throw std::runtime_error("OpenCL preview failed"); }

// Run in an executable so GPU cleanup happens outside Windows' DLL loader lock.
class Gpu {
public:
    using Handle = void*;
    using UInt = unsigned int;
    using Bits = unsigned long long;
    Handle context = nullptr, queue = nullptr;
    int (__stdcall *getPlatforms)(UInt, Handle*, UInt*) = nullptr;
    int (__stdcall *getDevices)(Handle, Bits, UInt, Handle*, UInt*) = nullptr;
    Handle (__stdcall *createContext)(const intptr_t*, UInt, const Handle*, void*, void*, int*) = nullptr;
    Handle (__stdcall *createQueue)(Handle, Handle, Bits, int*) = nullptr;
    Handle (__stdcall *createBuffer)(Handle, Bits, size_t, void*, int*) = nullptr;
    int (__stdcall *readBuffer)(Handle, Handle, UInt, size_t, size_t, void*, UInt, const Handle*, Handle*) = nullptr;
    int (__stdcall *finish)(Handle) = nullptr;
    int (__stdcall *releaseMem)(Handle) = nullptr;
    int (__stdcall *releaseQueue)(Handle) = nullptr;
    int (__stdcall *releaseContext)(Handle) = nullptr;

    Gpu()
    {
        HMODULE dll = LoadLibraryA("OpenCL.dll");
        if (!dll) return;
#define LOAD(member, symbol) member = reinterpret_cast<decltype(member)>(GetProcAddress(dll, symbol)); if (!member) return
        LOAD(getPlatforms, "clGetPlatformIDs");
        LOAD(getDevices, "clGetDeviceIDs");
        LOAD(createContext, "clCreateContext");
        LOAD(createQueue, "clCreateCommandQueue");
        LOAD(createBuffer, "clCreateBuffer");
        LOAD(readBuffer, "clEnqueueReadBuffer");
        LOAD(finish, "clFinish");
        LOAD(releaseMem, "clReleaseMemObject");
        LOAD(releaseQueue, "clReleaseCommandQueue");
        LOAD(releaseContext, "clReleaseContext");
#undef LOAD
        UInt count = 0;
        if (getPlatforms(0, nullptr, &count) != 0 || count == 0) return;
        std::vector<Handle> platforms(count);
        if (getPlatforms(count, platforms.data(), nullptr) != 0) return;
        Handle device = nullptr;
        for (Handle platform : platforms) {
            if (getDevices(platform, 4, 1, &device, nullptr) == 0) break;
            device = nullptr;
        }
        if (!device) return;
        int error = 0;
        context = createContext(nullptr, 1, &device, nullptr, nullptr, &error);
        if (error != 0 || !context) return;
        queue = createQueue(context, device, 0, &error);
        if (error != 0) queue = nullptr;
    }

    ~Gpu()
    {
        if (queue) { finish(queue); releaseQueue(queue); }
        if (context) releaseContext(context);
        // Keep the ICD loaded until process exit, as the production cache holds it too.
    }
};

struct Buffer {
    Gpu& gpu;
    Gpu::Handle handle = nullptr;
    ~Buffer() { if (handle) gpu.releaseMem(handle); }
};
}

static int renderFull(
    const float* input, float* output, int width, int height, double time,
    const float* settings, size_t settingCount)
try
{
    if (!input || !output || !settings || width < 1 || height < 1 || width > 16384 || height > 16384 ||
        !std::isfinite(time) || settingCount != film::SettingsCount) return 1;
    for (size_t i = 0; i < settingCount; ++i) if (!std::isfinite(settings[i])) return 1;
    if (settings[0] < 0 || settings[0] > 6 || settings[1] < 0 || settings[1] > 5 ||
        settings[2] < 0 || settings[2] > 3 || settings[film::ModuleIndex] < 0 || settings[film::ModuleIndex] > film::All ||
        settings[26] < 0 || settings[26] >= color::SpaceCount ||
        settings[27] < 0 || settings[27] >= color::OutputSpaces.size()) return 1;
    static Gpu gpu;
    if (!gpu.queue) return 2;
    const size_t bytes = static_cast<size_t>(width) * height * 4 * sizeof(float);
    int error = 0;
    Buffer source {gpu}, destination {gpu};
    source.handle = gpu.createBuffer(gpu.context, 4 | 32, bytes, const_cast<float*>(input), &error);
    require(error == 0 && source.handle);
    destination.handle = gpu.createBuffer(gpu.context, 1, bytes, nullptr, &error);
    require(error == 0 && destination.handle);
    const bool rendered = RunOpenEmulsionOpenCL(gpu.queue, width, height, time, settings,
        reinterpret_cast<const float*>(source.handle), reinterpret_cast<float*>(destination.handle));
    // Finish even after a failed dispatch before releasing its buffers.
    const int finished = gpu.finish(gpu.queue);
    require(rendered && finished == 0);
    require(gpu.readBuffer(gpu.queue, destination.handle, 1, 0, bytes, output, 0, nullptr, nullptr) == 0);
    return 0;
}
catch (...) { return 3; }

static std::vector<float> readFloats(const char* path, size_t count)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    require(file && file.tellg() == static_cast<std::streamoff>(count * sizeof(float)));
    file.seekg(0);
    std::vector<float> values(count);
    require(static_cast<bool>(file.read(reinterpret_cast<char*>(values.data()), count * sizeof(float))));
    for (float value : values) require(std::isfinite(value));
    return values;
}

int main(int argc, char** argv)
try
{
    if (argc != 7) {
        std::fprintf(stderr, "Usage: FullFrameBench input.f32 output.f32 width height settings.f32 time\n");
        return 1;
    }
    const int width = std::stoi(argv[3]), height = std::stoi(argv[4]);
    const double time = std::stod(argv[6]);
    require(width > 0 && height > 0 && width <= 16384 && height <= 16384 && std::isfinite(time));
    const size_t count = static_cast<size_t>(width) * height * 4;
    const auto input = readFloats(argv[1], count);
    const auto settings = readFloats(argv[5], film::SettingsCount);
    std::vector<float> output(count);
    const int status = renderFull(input.data(), output.data(), width, height, time, settings.data(), settings.size());
    if (status) {
        std::fprintf(stderr, "Production OpenCL render failed (%d); a working OpenCL GPU is required.\n", status);
        return status;
    }
    for (size_t i = 0; i < count; ++i) {
        require(std::isfinite(output[i]));
        if (i % 4 == 3) require(output[i] == input[i]);
    }
    std::ofstream file(argv[2], std::ios::binary);
    require(static_cast<bool>(file.write(reinterpret_cast<const char*>(output.data()), count * sizeof(float))));
    file.close();
    require(static_cast<bool>(file));
    return 0;
}
catch (const std::exception& error) {
    std::fprintf(stderr, "FullFrameBench: %s\n", error.what());
    return 4;
}
