#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

struct AtlasMeshData
{
    std::vector<float> vertices;
    std::vector<std::uint32_t> faces;
};

struct AtlasPipelineSnapshot
{
    AtlasMeshData source;
    AtlasMeshData parameterized;
    std::string phase;
    int step = 0;
    double distortion = 0.0;
    bool complete = false;
};

class AtlasPipeline
{
public:
    using ProgressCallback = std::function<void(const AtlasPipelineSnapshot&)>;

    AtlasPipeline();
    ~AtlasPipeline();

    AtlasPipeline(const AtlasPipeline&) = delete;
    AtlasPipeline& operator=(const AtlasPipeline&) = delete;

    void initialize(const std::string& inputPath);
    void executeNextStep(const ProgressCallback& progressCallback = {});
    bool isComplete() const;
    std::string nextStepName() const;
    AtlasPipelineSnapshot snapshot() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
