// SPDX-License-Identifier: MPL-2.0

#include <Windows.h>

#include <map>
#include <memory>
#include <mutex>
#include <stdio.h>
#include <tuple>

#include "HalationBlur.h"
#include "BloomBlur.h"
#include "BloomOpenCL.h"
#include "HalationConfig.h"
#include "HalationOpenCL.h"
#include "GrainConfig.h"
#include "GrainOpenCL.h"
#include "ColorSpaceConfig.h"
#include "ColorOpenCL.h"
#include "FilmResponseConfig.h"
#include "FilmResponseOpenCL.h"

typedef int cl_int;
typedef unsigned int cl_uint;
typedef void* cl_command_queue;
typedef void* cl_context;
typedef void* cl_device_id;
typedef void* cl_program;
typedef void* cl_kernel;
typedef void* cl_mem;
typedef void* cl_event;
typedef unsigned long long cl_mem_flags;

static const cl_int CL_SUCCESS = 0;
static const cl_uint CL_QUEUE_CONTEXT = 0x1090;
static const cl_uint CL_QUEUE_DEVICE = 0x1091;
static const cl_mem_flags CL_MEM_READ_WRITE = 1;
static const cl_mem_flags CL_MEM_READ_ONLY = 4;
static const cl_mem_flags CL_MEM_COPY_HOST_PTR = 32;

typedef cl_int(__stdcall* clGetCommandQueueInfoProc)(cl_command_queue, cl_uint, size_t, void*, size_t*);
typedef cl_program(__stdcall* clCreateProgramWithSourceProc)(cl_context, cl_uint, const char**, const size_t*, cl_int*);
typedef cl_int(__stdcall* clBuildProgramProc)(cl_program, cl_uint, const cl_device_id*, const char*, void*, void*);
typedef cl_kernel(__stdcall* clCreateKernelProc)(cl_program, const char*, cl_int*);
typedef cl_int(__stdcall* clSetKernelArgProc)(cl_kernel, cl_uint, size_t, const void*);
typedef cl_int(__stdcall* clEnqueueNDRangeKernelProc)(cl_command_queue, cl_kernel, cl_uint, const size_t*, const size_t*, const size_t*, cl_uint, const cl_event*, cl_event*);
typedef cl_mem(__stdcall* clCreateBufferProc)(cl_context, cl_mem_flags, size_t, void*, cl_int*);
typedef cl_int(__stdcall* clReleaseMemObjectProc)(cl_mem);
typedef cl_int(__stdcall* clReleaseEventProc)(cl_event);
typedef cl_int(__stdcall* clReleaseKernelProc)(cl_kernel);
typedef cl_int(__stdcall* clReleaseProgramProc)(cl_program);
typedef cl_int(__stdcall* clGetProgramBuildInfoProc)(cl_program, cl_device_id, cl_uint, size_t, void*, size_t*);

struct OpenCLApi {
    HMODULE dll = nullptr;
    clGetCommandQueueInfoProc clGetCommandQueueInfo = nullptr;
    clCreateProgramWithSourceProc clCreateProgramWithSource = nullptr;
    clBuildProgramProc clBuildProgram = nullptr;
    clCreateKernelProc clCreateKernel = nullptr;
    clSetKernelArgProc clSetKernelArg = nullptr;
    clEnqueueNDRangeKernelProc clEnqueueNDRangeKernel = nullptr;
    clCreateBufferProc clCreateBuffer = nullptr;
    clReleaseMemObjectProc clReleaseMemObject = nullptr;
    clReleaseEventProc clReleaseEvent = nullptr;
    clReleaseKernelProc clReleaseKernel = nullptr;
    clReleaseProgramProc clReleaseProgram = nullptr;
    clGetProgramBuildInfoProc clGetProgramBuildInfo = nullptr;
    bool ready = false;
};

static const char* KernelSource = R"CLC(
float lum(float3 c) { return c.x * 0.2126f + c.y * 0.7152f + c.z * 0.0722f; }
float clamp01(float v) { return clamp(v, 0.0f, 1.0f); }

float3 read_rgb(__global const float* input, int width, int height, int x, int y)
{
    x = clamp(x, 0, width - 1);
    y = clamp(y, 0, height - 1);
    int idx = ((y * width) + x) * 4;
    return (float3)(input[idx], input[idx + 1], input[idx + 2]);
}

float highlight_key(float3 c, HalationParameters p)
{
    return halation_key(c.x, c.y, c.z, p);
}

float3 to_work(float3 v, ColorParameters p)
{
    ColorRgb rgb = {v.x, v.y, v.z};
    rgb = color_to_work(rgb, p);
    return (float3)(rgb.r, rgb.g, rgb.b);
}

float3 from_work(float3 v, ColorParameters p)
{
    ColorRgb rgb = {v.x, v.y, v.z};
    rgb = color_from_work(rgb, p);
    return (float3)(rgb.r, rgb.g, rgb.b);
}

__kernel void ExtractHighlights(int width, int height, int step, ColorParameters color, HalationParameters halo,
                                __global const float* input, __global float2* output)
{
    int x = get_global_id(0), y = get_global_id(1);
    int bw = (width + step - 1) / step, bh = (height + step - 1) / step;
    if (x >= bw || y >= bh) return;
    float sum = 0.0f;
    int count = 0;
    for (int dy = 0; dy < step && step * y + dy < height; ++dy) {
        for (int dx = 0; dx < step && step * x + dx < width; ++dx) {
            sum += highlight_key(to_work(read_rgb(input, width, height, step * x + dx, step * y + dy), color), halo);
            ++count;
        }
    }
    output[y * bw + x] = (float2)(sum / count);
}

__kernel void BlurHighlights(int width, int height, int radius, int horizontal,
                             __constant const float2* weights, __global const float2* input, __global float2* output)
{
    int x = get_global_id(0), y = get_global_id(1);
    if (x >= width || y >= height) return;
    float2 sum = (float2)(0.0f);
    for (int i = -radius; i <= radius; ++i) {
        int sx = horizontal ? clamp(x + i, 0, width - 1) : x;
        int sy = horizontal ? y : clamp(y + i, 0, height - 1);
        sum += input[sy * width + sx] * weights[i + radius];
    }
    output[y * width + x] = sum;
}

float2 sample_blur(__global const float2* image, int width, int height, int x, int y, int step)
{
    float fx = clamp(((float)x - (step - 1) * 0.5f) / step, 0.0f, (float)(width - 1));
    float fy = clamp(((float)y - (step - 1) * 0.5f) / step, 0.0f, (float)(height - 1));
    int x0 = (int)fx, y0 = (int)fy;
    int x1 = min(x0 + 1, width - 1), y1 = min(y0 + 1, height - 1);
    float2 top = mix(image[y0 * width + x0], image[y0 * width + x1], fx - x0);
    float2 bottom = mix(image[y1 * width + x0], image[y1 * width + x1], fx - x0);
    return mix(top, bottom, fy - y0);
}

__kernel void ExtractBloom(int width, int height, int step, ColorParameters color, BloomParameters parameters,
                           __global const float* input, __global float4* output)
{
    int x = get_global_id(0), y = get_global_id(1);
    int bw = (width + step - 1) / step, bh = (height + step - 1) / step;
    if (x >= bw || y >= bh) return;
    float4 sum = (float4)(0.0f); int count = 0;
    for (int dy = 0; dy < step && y * step + dy < height; ++dy)
        for (int dx = 0; dx < step && x * step + dx < width; ++dx) {
            float3 rgb = read_rgb(input,width,height,x*step+dx,y*step+dy);
            ColorRgb c = bloom_extract((ColorRgb){rgb.x,rgb.y,rgb.z},color,parameters);
            sum += (float4)(c.r,c.g,c.b,0.0f); ++count;
        }
    output[y * bw + x] = sum / (float)count;
}

__kernel void BlurBloom(int width, int height, int radius, int horizontal,
                        __constant const float* weights, __global const float4* input, __global float4* output)
{
    int x = get_global_id(0), y = get_global_id(1);
    if (x >= width || y >= height) return;
    float4 sum = (float4)(0.0f);
    for (int i = -radius; i <= radius; ++i) {
        int sx = horizontal ? clamp(x+i,0,width-1) : x;
        int sy = horizontal ? y : clamp(y+i,0,height-1);
        sum += input[sy*width+sx] * weights[i+radius];
    }
    output[y*width+x] = sum;
}

float4 sample_bloom(__global const float4* image, int width, int height, int x, int y, int step)
{
    float fx = clamp(((float)x - (step-1)*0.5f)/step,0.0f,(float)(width-1));
    float fy = clamp(((float)y - (step-1)*0.5f)/step,0.0f,(float)(height-1));
    int x0 = (int)fx, y0 = (int)fy;
    int x1 = min(x0+1,width-1), y1 = min(y0+1,height-1);
    float4 top = mix(image[y0*width+x0],image[y0*width+x1],fx-x0);
    float4 bottom = mix(image[y1*width+x0],image[y1*width+x1],fx-x0);
    return mix(top,bottom,fy-y0);
}

__kernel void OpenEmulsionKernel(
    int width,
    int height,
    int modules,
    int mode,
    float gainR,
    float gainG,
    float gainB,
    float halation,
    float aura,
    float grain,
    GrainParameters grainParameters,
    ColorParameters color,
    FilmResponseParameters response,
    HalationParameters haloParameters,
    BloomParameters bloomParameters,
    int bloomStep,
    int step,
    int identity,
    __global const float* input,
    __global float* output,
    __global const float2* blurred,
    __global const float4* bloomed)
{
    int x = get_global_id(0);
    int y = get_global_id(1);
    if (x >= width || y >= height) return;

    int idx = ((y * width) + x) * 4;
    float3 original = (float3)(input[idx], input[idx + 1], input[idx + 2]);
    float alpha = input[idx + 3];
    if (identity) {
        output[idx] = original.x; output[idx + 1] = original.y; output[idx + 2] = original.z; output[idx + 3] = alpha;
        return;
    }
    float3 work = to_work(original, color);
    float3 c = work;

    if (modules & 1) {
        ColorRgb rgb = {c.x, c.y, c.z}, gain = {gainR, gainG, gainB};
        rgb = color_balance(rgb, gain);
        rgb = response_negative_stage(rgb, response);
        c = (float3)(rgb.r, rgb.g, rgb.b);
    }
    if (modules & 32) {
        ColorRgb rgb = response_development((ColorRgb){c.x, c.y, c.z}, response);
        c = (float3)(rgb.r, rgb.g, rgb.b);
    }
    if (modules & 2) {
        ColorRgb rgb = {c.x, c.y, c.z};
        rgb = response_print(rgb, response);
        c = (float3)(rgb.r, rgb.g, rgb.b);
    }

    if (halation > 0.0f || aura > 0.0f) {
        float2 halo = sample_blur(blurred, (width + step - 1) / step, (height + step - 1) / step, x, y, step);
        float center = highlight_key(work, haloParameters);
        float h = halation_signal(halo.x, halo.y, center, halation, aura);
        if (mode == 5) {
            h = clamp(h * 2.0f, 0.0f, 1.0f);
            c = (float3)(h, h * 0.55f, h * 0.10f);
        } else {
            c += (float3)(h * haloParameters.red, h * haloParameters.green, h * haloParameters.blue);
        }
    } else if (mode == 5) {
        c = (float3)(0.0f);
    }

    if (bloomParameters.amount > 0.0f || mode == 6) {
        float4 glow = bloomParameters.amount > 0.0f ?
            sample_bloom(bloomed, (width + bloomStep - 1) / bloomStep, (height + bloomStep - 1) / bloomStep, x, y, bloomStep) : (float4)(0.0f);
        ColorRgb rgb = bloom_composite((ColorRgb){c.x,c.y,c.z}, (ColorRgb){glow.x,glow.y,glow.z}, bloomParameters, mode == 6);
        c = (float3)(rgb.r,rgb.g,rgb.b);
    }
    if ((modules & 16) && grain > 0.0f) {
        GrainVector delta = grain_delta(x, y, lum(c), grainParameters);
        c += (float3)(delta.r, delta.g, delta.b);
    }

    ColorRgb finished = response_finish((ColorRgb){c.x, c.y, c.z}, modules, response);
    c = (float3)(finished.r, finished.g, finished.b);
    if (response_clamps_negative(modules, response)) c = fmax(c, (float3)(0.0f));
    if (mode != 5 && mode != 6) c = !(modules & 35) && all(c == work) ? original : from_work(c, color);
    output[idx] = c.x;
    output[idx + 1] = c.y;
    output[idx + 2] = c.z;
    output[idx + 3] = alpha;
}
)CLC";

static bool check(cl_int error, const char* msg)
{
    if (error != CL_SUCCESS) {
        fprintf(stderr, "%s [%d]\n", msg, error);
        return false;
    }
    return true;
}

static OpenCLApi* api()
{
    static OpenCLApi a;
    static std::once_flag once;
    std::call_once(once, [&] {
        a.dll = LoadLibraryA("OpenCL.dll");
        if (!a.dll) return;
#define LOAD_CL(name) a.name = reinterpret_cast<name##Proc>(GetProcAddress(a.dll, #name)); if (!a.name) return
        LOAD_CL(clGetCommandQueueInfo);
        LOAD_CL(clCreateProgramWithSource);
        LOAD_CL(clBuildProgram);
        LOAD_CL(clCreateKernel);
        LOAD_CL(clSetKernelArg);
        LOAD_CL(clEnqueueNDRangeKernel);
        LOAD_CL(clCreateBuffer);
        LOAD_CL(clReleaseMemObject);
        LOAD_CL(clReleaseEvent);
        LOAD_CL(clReleaseKernel);
        LOAD_CL(clReleaseProgram);
        LOAD_CL(clGetProgramBuildInfo);
#undef LOAD_CL
        a.ready = true;
    });
    return a.ready ? &a : nullptr;
}

struct OpenCLResources {
    OpenCLApi* cl;
    cl_program program = nullptr;
    cl_kernel film = nullptr, extract = nullptr, blur = nullptr, extractBloom = nullptr, blurBloom = nullptr;
    cl_mem highlights = nullptr, temporary = nullptr;
    size_t capacity = 0, bloomCapacity = 0;
    cl_mem bloomHighlights = nullptr, bloomTemporary = nullptr;
    cl_event tail = nullptr;

    explicit OpenCLResources(OpenCLApi* api) : cl(api) {}
    ~OpenCLResources()
    {
        if (tail) cl->clReleaseEvent(tail);
        if (highlights) cl->clReleaseMemObject(highlights);
        if (temporary) cl->clReleaseMemObject(temporary);
        if (bloomHighlights) cl->clReleaseMemObject(bloomHighlights);
        if (bloomTemporary) cl->clReleaseMemObject(bloomTemporary);
        if (extractBloom) cl->clReleaseKernel(extractBloom);
        if (blurBloom) cl->clReleaseKernel(blurBloom);
        if (film) cl->clReleaseKernel(film);
        if (extract) cl->clReleaseKernel(extract);
        if (blur) cl->clReleaseKernel(blur);
        if (program) cl->clReleaseProgram(program);
    }

    bool enqueue(cl_command_queue queue, cl_kernel kernel, int width, int height)
    {
        const size_t global[2] = {static_cast<size_t>(width), static_cast<size_t>(height)};
        cl_event next = nullptr;
        // Explicit dependencies protect reused scratch buffers even on out-of-order queues.
        const cl_int error = cl->clEnqueueNDRangeKernel(queue, kernel, 2, nullptr, global, nullptr,
                                                      tail ? 1 : 0, tail ? &tail : nullptr, &next);
        if (!check(error, "Unable to enqueue OpenCL kernel")) return false;
        if (tail) cl->clReleaseEvent(tail);
        tail = next;
        return true;
    }
};

struct OpenCLBuffer {
    OpenCLApi* cl;
    cl_mem mem;
    ~OpenCLBuffer() { if (mem) cl->clReleaseMemObject(mem); }
};

bool RunOpenEmulsionOpenCL(void* cmdQueue, int width, int height, double time, const float* settings, const float* input, float* output)
{
    OpenCLApi* cl = api();
    if (!cl || !cmdQueue || width <= 0 || height <= 0) return false;

    cl_int error = CL_SUCCESS;
    cl_command_queue queue = static_cast<cl_command_queue>(cmdQueue);
    static std::mutex mutex;
    using Key = std::tuple<cl_context, cl_device_id, cl_command_queue>;
    static std::map<Key, std::unique_ptr<OpenCLResources>> resources;

    std::lock_guard<std::mutex> lock(mutex);

    cl_device_id device = nullptr;
    cl_context context = nullptr;
    error = cl->clGetCommandQueueInfo(queue, CL_QUEUE_DEVICE, sizeof(device), &device, nullptr);
    if (!check(error, "Unable to get OpenCL device")) return false;
    error = cl->clGetCommandQueueInfo(queue, CL_QUEUE_CONTEXT, sizeof(context), &context, nullptr);
    if (!check(error, "Unable to get OpenCL context")) return false;
    const Key key(context, device, queue);
    if (resources.find(key) == resources.end()) {
        auto created = std::make_unique<OpenCLResources>(cl);
        const char* sources[] = {GrainOpenCLSource, ColorOpenCLSource, FilmResponseOpenCLSource, HalationOpenCLSource, BloomOpenCLSource, KernelSource};
        created->program = cl->clCreateProgramWithSource(context, 6, sources, nullptr, &error);
        if (!check(error, "Unable to create OpenCL program")) return false;
        error = cl->clBuildProgram(created->program, 1, &device, nullptr, nullptr, nullptr);
        if (error != CL_SUCCESS) {
            size_t size = 0;
            const cl_uint CL_PROGRAM_BUILD_LOG = 0x1183;
            cl->clGetProgramBuildInfo(created->program, device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &size);
            std::vector<char> log(size + 1, 0);
            cl->clGetProgramBuildInfo(created->program, device, CL_PROGRAM_BUILD_LOG, size, log.data(), nullptr);
            fprintf(stderr, "OpenCL build log: %s\n", log.data());
            return check(error, "Unable to build OpenCL program");
        }
        created->film = cl->clCreateKernel(created->program, "OpenEmulsionKernel", &error);
        if (!check(error, "Unable to create film kernel")) return false;
        created->extract = cl->clCreateKernel(created->program, "ExtractHighlights", &error);
        if (!check(error, "Unable to create highlight kernel")) return false;
        created->blur = cl->clCreateKernel(created->program, "BlurHighlights", &error);
        if (!check(error, "Unable to create blur kernel")) return false;
        created->extractBloom = cl->clCreateKernel(created->program, "ExtractBloom", &error);
        if (!check(error, "Unable to create bloom extraction kernel")) return false;
        created->blurBloom = cl->clCreateKernel(created->program, "BlurBloom", &error);
        if (!check(error, "Unable to create bloom blur kernel")) return false;
        resources.emplace(key, std::move(created));
    }
    OpenCLResources& state = *resources.at(key);
    const cl_kernel kernel = state.film;

    cl_mem inputMem = reinterpret_cast<cl_mem>(const_cast<float*>(input));
    cl_mem outputMem = reinterpret_cast<cl_mem>(output);
    int mode = static_cast<int>(settings[0] + 0.5f);
    const int modules = film::modulesForSettings(settings);
    const float halation = (modules & film::Halation) ? settings[13] : 0.0f;
    const float aura = (modules & film::Aura) ? settings[15] : 0.0f;
    const GrainParameters grainParameters = grain::prepare(settings, height, time);
    const ColorParameters colorParameters = color::prepare(settings);
    const FilmResponseParameters responseParameters = response::prepare(settings);
    static_assert(sizeof(FilmResponseParameters) == 212, "OpenCL response structure layout mismatch");
    const auto haloConfig = halation::prepare(settings, height);
    const int step = haloConfig.downsample;
    static_assert(sizeof(HalationParameters) == 20, "OpenCL halation structure layout mismatch");
    const auto bloomConfig = bloom::prepare(settings, height);
    static_assert(sizeof(BloomParameters) == 20, "OpenCL bloom structure layout mismatch");
    static_assert(sizeof(bloom::Pixel) == 16, "OpenCL bloom float4 layout mismatch");
    const int identity = film::isIdentity(mode, modules, halation, aura, settings[16], bloomConfig.parameters.amount);
    static_assert(sizeof(ColorParameters) == 88, "OpenCL color structure layout mismatch");
    static_assert(sizeof(GrainParameters) == 56, "OpenCL grain structure layout mismatch");
    cl_mem blurMem = inputMem;
    if (halation > 0.0f || aura > 0.0f) {
        const int bw = (width + step - 1) / step, bh = (height + step - 1) / step;
        const size_t bytes = static_cast<size_t>(bw) * bh * sizeof(halation::Pair);
        if (state.capacity < bytes) {
            OpenCLBuffer highlights {cl, cl->clCreateBuffer(context, CL_MEM_READ_WRITE, bytes, nullptr, &error)};
            if (!check(error, "Unable to allocate highlight buffer")) return false;
            OpenCLBuffer temporary {cl, cl->clCreateBuffer(context, CL_MEM_READ_WRITE, bytes, nullptr, &error)};
            if (!check(error, "Unable to allocate blur buffer")) return false;
            if (state.highlights) cl->clReleaseMemObject(state.highlights);
            if (state.temporary) cl->clReleaseMemObject(state.temporary);
            state.highlights = highlights.mem;
            state.temporary = temporary.mem;
            highlights.mem = temporary.mem = nullptr;
            state.capacity = bytes;
        }
        const halation::Filter filter(halation, settings[14], aura, haloConfig.auraRadius, haloConfig.radiusScale);
        static_assert(sizeof(halation::Pair) == 2 * sizeof(float), "OpenCL float2 layout mismatch");
        OpenCLBuffer weights {cl, cl->clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                              filter.weights.size() * sizeof(halation::Pair), const_cast<halation::Pair*>(filter.weights.data()), &error)};
        if (!check(error, "Unable to upload halation weights")) return false;
        error  = cl->clSetKernelArg(state.extract, 0, sizeof(int), &width);
        error |= cl->clSetKernelArg(state.extract, 1, sizeof(int), &height);
        error |= cl->clSetKernelArg(state.extract, 2, sizeof(int), &step);
        error |= cl->clSetKernelArg(state.extract, 3, sizeof(ColorParameters), &colorParameters);
        error |= cl->clSetKernelArg(state.extract, 4, sizeof(HalationParameters), &haloConfig.key);
        error |= cl->clSetKernelArg(state.extract, 5, sizeof(cl_mem), &inputMem);
        error |= cl->clSetKernelArg(state.extract, 6, sizeof(cl_mem), &state.highlights);
        if (!check(error, "Unable to set highlight arguments") || !state.enqueue(queue, state.extract, bw, bh)) return false;
        for (int horizontal = 1; horizontal >= 0; --horizontal) {
            cl_mem src = horizontal ? state.highlights : state.temporary;
            cl_mem dst = horizontal ? state.temporary : state.highlights;
            error  = cl->clSetKernelArg(state.blur, 0, sizeof(int), &bw);
            error |= cl->clSetKernelArg(state.blur, 1, sizeof(int), &bh);
            error |= cl->clSetKernelArg(state.blur, 2, sizeof(int), &filter.radius);
            error |= cl->clSetKernelArg(state.blur, 3, sizeof(int), &horizontal);
            error |= cl->clSetKernelArg(state.blur, 4, sizeof(cl_mem), &weights.mem);
            error |= cl->clSetKernelArg(state.blur, 5, sizeof(cl_mem), &src);
            error |= cl->clSetKernelArg(state.blur, 6, sizeof(cl_mem), &dst);
            if (!check(error, "Unable to set blur arguments") || !state.enqueue(queue, state.blur, bw, bh)) return false;
        }
        blurMem = state.highlights;
    }

    cl_mem bloomMem = inputMem;
    if (bloomConfig.parameters.amount > 0.0f) {
        const int bloomStep = bloomConfig.downsample;
        const int bw = (width + bloomStep - 1) / bloomStep, bh = (height + bloomStep - 1) / bloomStep;
        const size_t bytes = static_cast<size_t>(bw) * bh * sizeof(bloom::Pixel);
        if (state.bloomCapacity < bytes) {
            OpenCLBuffer highlights {cl, cl->clCreateBuffer(context, CL_MEM_READ_WRITE, bytes, nullptr, &error)};
            if (!check(error, "Unable to allocate bloom buffer")) return false;
            OpenCLBuffer temporary {cl, cl->clCreateBuffer(context, CL_MEM_READ_WRITE, bytes, nullptr, &error)};
            if (!check(error, "Unable to allocate bloom temporary")) return false;
            if (state.bloomHighlights) cl->clReleaseMemObject(state.bloomHighlights);
            if (state.bloomTemporary) cl->clReleaseMemObject(state.bloomTemporary);
            state.bloomHighlights = highlights.mem; state.bloomTemporary = temporary.mem;
            highlights.mem = temporary.mem = nullptr;
            state.bloomCapacity = bytes;
        }
        const bloom::Filter filter(bloomConfig.sigma);
        OpenCLBuffer weights {cl, cl->clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
            filter.weights.size() * sizeof(float), const_cast<float*>(filter.weights.data()), &error)};
        if (!check(error, "Unable to upload bloom weights")) return false;
        error  = cl->clSetKernelArg(state.extractBloom, 0, sizeof(int), &width);
        error |= cl->clSetKernelArg(state.extractBloom, 1, sizeof(int), &height);
        error |= cl->clSetKernelArg(state.extractBloom, 2, sizeof(int), &bloomStep);
        error |= cl->clSetKernelArg(state.extractBloom, 3, sizeof(ColorParameters), &colorParameters);
        error |= cl->clSetKernelArg(state.extractBloom, 4, sizeof(BloomParameters), &bloomConfig.parameters);
        error |= cl->clSetKernelArg(state.extractBloom, 5, sizeof(cl_mem), &inputMem);
        error |= cl->clSetKernelArg(state.extractBloom, 6, sizeof(cl_mem), &state.bloomHighlights);
        if (!check(error, "Unable to set bloom extraction arguments") || !state.enqueue(queue, state.extractBloom, bw, bh)) return false;
        for (int horizontal = 1; horizontal >= 0; --horizontal) {
            cl_mem src = horizontal ? state.bloomHighlights : state.bloomTemporary;
            cl_mem dst = horizontal ? state.bloomTemporary : state.bloomHighlights;
            error  = cl->clSetKernelArg(state.blurBloom, 0, sizeof(int), &bw);
            error |= cl->clSetKernelArg(state.blurBloom, 1, sizeof(int), &bh);
            error |= cl->clSetKernelArg(state.blurBloom, 2, sizeof(int), &filter.radius);
            error |= cl->clSetKernelArg(state.blurBloom, 3, sizeof(int), &horizontal);
            error |= cl->clSetKernelArg(state.blurBloom, 4, sizeof(cl_mem), &weights.mem);
            error |= cl->clSetKernelArg(state.blurBloom, 5, sizeof(cl_mem), &src);
            error |= cl->clSetKernelArg(state.blurBloom, 6, sizeof(cl_mem), &dst);
            if (!check(error, "Unable to set bloom blur arguments") || !state.enqueue(queue, state.blurBloom, bw, bh)) return false;
        }
        bloomMem = state.bloomHighlights;
    }

    int arg = 0;
    error  = cl->clSetKernelArg(kernel, arg++, sizeof(int), &width);
    error |= cl->clSetKernelArg(kernel, arg++, sizeof(int), &height);
    error |= cl->clSetKernelArg(kernel, arg++, sizeof(int), &modules);
    error |= cl->clSetKernelArg(kernel, arg++, sizeof(int), &mode);
    for (int i = 4; i < 7; ++i) {
        const float value = settings[i];
        error |= cl->clSetKernelArg(kernel, arg++, sizeof(float), &value);
    }
    error |= cl->clSetKernelArg(kernel, arg++, sizeof(float), &halation);
    error |= cl->clSetKernelArg(kernel, arg++, sizeof(float), &aura);
    error |= cl->clSetKernelArg(kernel, arg++, sizeof(float), &settings[16]);
    error |= cl->clSetKernelArg(kernel, arg++, sizeof(GrainParameters), &grainParameters);
    error |= cl->clSetKernelArg(kernel, arg++, sizeof(ColorParameters), &colorParameters);
    error |= cl->clSetKernelArg(kernel, arg++, sizeof(FilmResponseParameters), &responseParameters);
    error |= cl->clSetKernelArg(kernel, arg++, sizeof(HalationParameters), &haloConfig.key);
    error |= cl->clSetKernelArg(kernel, arg++, sizeof(BloomParameters), &bloomConfig.parameters);
    error |= cl->clSetKernelArg(kernel, arg++, sizeof(int), &bloomConfig.downsample);
    error |= cl->clSetKernelArg(kernel, arg++, sizeof(int), &step);
    error |= cl->clSetKernelArg(kernel, arg++, sizeof(int), &identity);
    error |= cl->clSetKernelArg(kernel, arg++, sizeof(cl_mem), &inputMem);
    error |= cl->clSetKernelArg(kernel, arg++, sizeof(cl_mem), &outputMem);
    error |= cl->clSetKernelArg(kernel, arg++, sizeof(cl_mem), &blurMem);
    error |= cl->clSetKernelArg(kernel, arg++, sizeof(cl_mem), &bloomMem);
    if (!check(error, "Unable to set OpenCL kernel arguments")) return false;

    return state.enqueue(queue, kernel, width, height);
}
