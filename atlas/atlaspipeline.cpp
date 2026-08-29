#include "atlaspipeline.h"

#include "ParaQuadCutting.h"
#include "PolySquareDeformation.h"
#include "MeshViewer/Mesh_doubleIO.h"
#include "Scaffold/StateManager.h"

#include <Eigen/Core>

#include <algorithm>
#include <filesystem>
#include <map>
#include <stdexcept>
#include <utility>

namespace
{
AtlasMeshData copyMeshData(const Mesh& mesh)
{
    AtlasMeshData data;
    data.vertices.reserve(mesh.n_vertices() * 3);
    for (auto vertex : mesh.vertices()) {
        const auto& point = mesh.point(vertex);
        data.vertices.push_back(float(point[0]));
        data.vertices.push_back(float(point[1]));
        data.vertices.push_back(float(point[2]));
    }

    data.faces.reserve(mesh.n_faces() * 3);
    for (auto face : mesh.faces()) {
        std::vector<std::uint32_t> vertices;
        for (auto vertex : mesh.fv_range(face))
            vertices.push_back(std::uint32_t(vertex.idx()));
        for (std::size_t i = 1; i + 1 < vertices.size(); ++i) {
            data.faces.push_back(vertices[0]);
            data.faces.push_back(vertices[i]);
            data.faces.push_back(vertices[i + 1]);
        }
    }
    return data;
}

AtlasMeshData copyTexturedMeshData(const Mesh& mesh, std::size_t textureVertexCount)
{
    OpenMesh::HPropHandleT<int> textureIndex;
    if (!mesh.get_property_handle(textureIndex, "hvt_index"))
        return copyMeshData(mesh);

    AtlasMeshData data;
    data.vertices.assign(textureVertexCount * 3, 0.0f);
    data.faces.reserve(mesh.n_faces() * 3);
    std::vector<bool> assigned(textureVertexCount, false);

    for (auto face : mesh.faces()) {
        std::vector<std::uint32_t> vertices;
        for (auto halfedge : mesh.fh_range(face)) {
            const int textureVertex = mesh.property(textureIndex, halfedge);
            if (textureVertex < 0 || std::size_t(textureVertex) >= textureVertexCount)
                return copyMeshData(mesh);

            const auto& point = mesh.point(mesh.to_vertex_handle(halfedge));
            const std::size_t offset = std::size_t(textureVertex) * 3;
            data.vertices[offset] = float(point[0]);
            data.vertices[offset + 1] = float(point[1]);
            data.vertices[offset + 2] = float(point[2]);
            assigned[std::size_t(textureVertex)] = true;
            vertices.push_back(std::uint32_t(textureVertex));
        }
        for (std::size_t i = 1; i + 1 < vertices.size(); ++i) {
            data.faces.push_back(vertices[0]);
            data.faces.push_back(vertices[i]);
            data.faces.push_back(vertices[i + 1]);
        }
    }

    if (std::find(assigned.begin(), assigned.end(), false) != assigned.end())
        return copyMeshData(mesh);
    return data;
}

void prepareMesh(Mesh& mesh)
{
    mesh.request_vertex_status();
    mesh.request_edge_status();
    mesh.request_face_status();
    mesh.request_face_normals();
    mesh.request_vertex_normals();
    mesh.update_normals();

    OpenMesh::EPropHandleT<bool> edgeOnCut;
    if (!mesh.get_property_handle(edgeOnCut, "e_oncut"))
        mesh.add_property(edgeOnCut, "e_oncut");
    for (auto edge : mesh.edges())
        mesh.property(edgeOnCut, edge) = mesh.is_boundary(edge);
}
}

class AtlasPipeline::Impl
{
public:
    enum class Stage
    {
        Empty,
        Loaded,
        Preprocessed,
        PolySquareInitialization,
        PolySquareIteration,
        PolySquareComplete,
        Packed,
        Complete
    };

    void initialize(const std::string& path)
    {
        inputPath = path;
        const std::filesystem::path filesystemPath(path);
        inputDirectory = filesystemPath.parent_path().string();
        inputFile = filesystemPath.string();

        mesh.clear();
        prepareMesh(mesh);
        if (!Mesh_doubleIO::load_mesh(mesh, inputFile.c_str(), true))
            throw std::runtime_error("Could not load the atlas input mesh.");
        if (mesh.vertices_empty() || mesh.faces_empty())
            throw std::runtime_error("The atlas input mesh is empty.");
        for (auto face : mesh.faces())
            if (mesh.valence(face) != 3)
                throw std::runtime_error("AAAtlas requires a triangular mesh.");

        prepareMesh(mesh);
        source = copyMeshData(mesh);
        boundaryVk.clear();
        polySquare.reset();
        reloadCutting();
        inputDistortion = cutting->get_distortion();
        phase = "Loaded input UV atlas";
        stepIndex = 0;
        stage = Stage::Loaded;
    }

    void executeNextStep(const AtlasPipeline::ProgressCallback& progressCallback)
    {
        switch (stage) {
        case Stage::Loaded:
            preprocess();
            phase = "Pre-Process";
            stage = Stage::Preprocessed;
            break;
        case Stage::Preprocessed:
            beginPolySquare();
            phase = "PolySquare Initialization";
            stage = Stage::PolySquareInitialization;
            break;
        case Stage::PolySquareInitialization:
        case Stage::PolySquareIteration:
            advancePolySquare();
            break;
        case Stage::PolySquareComplete:
            cutAndPack();
            phase = "Quad Cut and Pack";
            stage = Stage::Packed;
            break;
        case Stage::Packed:
            reduceDistortion(progressCallback);
            phase = "Distortion Reduce";
            stage = Stage::Complete;
            break;
        case Stage::Empty:
            throw std::runtime_error("Load an atlas mesh before running the pipeline.");
        case Stage::Complete:
            return;
        }
        ++stepIndex;
    }

    AtlasPipelineSnapshot snapshot() const
    {
        AtlasPipelineSnapshot result;
        result.source = source;
        if (cutting && cutting->is_valid()) {
            result.parameterized = copyMeshData(cutting->get_origin_para());
            result.source = copyTexturedMeshData(
                mesh, result.parameterized.vertices.size() / 3);
        }
        result.phase = phase;
        result.step = stepIndex;
        result.distortion = cutting ? cutting->get_distortion() : 0.0;
        result.complete = stage == Stage::Complete;
        return result;
    }

    bool isComplete() const { return stage == Stage::Complete; }

    std::string nextStepName() const
    {
        switch (stage) {
        case Stage::Loaded:
            return "Pre-Process";
        case Stage::Preprocessed:
            return "PolySquare Initialization";
        case Stage::PolySquareInitialization:
        case Stage::PolySquareIteration:
            return "PolySquare Iteration";
        case Stage::PolySquareComplete:
            return "Quad Cut and Pack";
        case Stage::Packed:
            return "Distortion Reduce";
        case Stage::Empty:
            return "Load Atlas";
        case Stage::Complete:
            return "Complete";
        }
        return {};
    }

private:
    void reloadCutting()
    {
        cutting = std::make_unique<ParaQuadCutting>(
            mesh, inputDirectory.c_str(), inputFile.c_str());
        if (!cutting->is_valid())
            throw std::runtime_error(
                "AAAtlas requires an OBJ with valid per-corner texture coordinates (vt).");
        cutting->calc_distortion(true);
    }

    void preprocess()
    {
        cutting->flip_neg_charts();
        cutting->get_textured_mesh(mesh);
        prepareMesh(mesh);
        cutting.reset();
        polySquare.reset();
        boundaryVk.clear();
        reloadCutting();

        OpenMesh::EPropHandleT<bool> edgeOnCut;
        if (!mesh.get_property_handle(edgeOnCut, "e_oncut"))
            throw std::runtime_error("The AAAtlas cut-edge property is unavailable.");

        std::vector<bool> vertexOnCut(mesh.n_vertices(), false);
        for (auto edge : mesh.edges()) {
            if (!mesh.property(edgeOnCut, edge))
                continue;
            const auto first = mesh.to_vertex_handle(mesh.halfedge_handle(edge, 0));
            const auto second = mesh.to_vertex_handle(mesh.halfedge_handle(edge, 1));
            vertexOnCut[first.idx()] = true;
            vertexOnCut[second.idx()] = true;
        }

        std::vector<int> bridgeEdges;
        for (auto edge : mesh.edges()) {
            if (mesh.property(edgeOnCut, edge))
                continue;
            const auto first = mesh.to_vertex_handle(mesh.halfedge_handle(edge, 0));
            const auto second = mesh.to_vertex_handle(mesh.halfedge_handle(edge, 1));
            if (vertexOnCut[first.idx()] && vertexOnCut[second.idx()])
                bridgeEdges.push_back(edge.idx());
        }

        if (!bridgeEdges.empty()) {
            cutting->split_edges(bridgeEdges);
            cutting->get_textured_mesh(mesh);
            prepareMesh(mesh);
            cutting.reset();
            boundaryVk.clear();
            reloadCutting();
        }
    }

    void beginPolySquare()
    {
        const double goalLength =
            (cutting->get_BB_Max() - cutting->get_BB_Min()).norm() / 3000.0;
        if (!(goalLength > 0.0))
            throw std::runtime_error("The input UV bounding box is degenerate.");

        polySquare = std::make_unique<PolySquareDeformation>(
            mesh, cutting->get_origin_para(), boundaryVk);
        polySquare->set_path(inputDirectory.c_str());
        if (!polySquare->begin_calc(kernelWidth, goalLength, energyExponent)) {
            polySquare.reset();
            throw std::runtime_error(
                "PolySquare initialization failed; charts may contain holes or flipped faces.");
        }
    }

    void advancePolySquare()
    {
        std::string title;
        bool enteredGridScale = false;
        if (!polySquare || !polySquare->step(title, enteredGridScale))
            throw std::runtime_error("PolySquare could not execute the next step.");

        polySquare->set_phase(polySquare->get_n_phase() - 1);
        cutting->update_para();
        cutting->update_textured_mesh(mesh);
        cutting->calc_distortion(true);
        phase = "PolySquare " + title;
        stage = polySquare->is_complete()
            ? Stage::PolySquareComplete
            : Stage::PolySquareIteration;
    }

    void cutAndPack()
    {
        cutting->cutting(splitThreshold);
        cutting->trans_textured(mesh);
        prepareMesh(mesh);
        polySquare.reset();
        cutting.reset();
        boundaryVk.clear();
        reloadCutting();
    }

    void applyDistortionResult(const Eigen::MatrixXd& uvPositions)
    {
        cutting->load_from_scaf(uvPositions);
        cutting->update_textured_mesh(mesh, true);
        prepareMesh(mesh);
        cutting.reset();
        boundaryVk.clear();
        reloadCutting();
    }

    void reduceDistortion(const AtlasPipeline::ProgressCallback& progressCallback)
    {
        Eigen::MatrixXd positions;
        Eigen::MatrixXd uvPositions;
        Eigen::MatrixXi faces;
        Eigen::MatrixXi uvFaces;
        cutting->get_scaf_info(positions, uvPositions, faces, uvFaces);

        StateManager stateManager;
        stateManager.run_interface(
            positions, uvPositions, faces, uvFaces,
            inputDistortion * 4.0, chartGap, packingEfficiencyBound, "LOWER",
            [this, &progressCallback](const Eigen::MatrixXd& currentUv, int iteration) {
                applyDistortionResult(currentUv);
                phase = "Distortion Reduce Iteration " + std::to_string(iteration);
                if (progressCallback)
                    progressCallback(snapshot());
            });
        applyDistortionResult(stateManager.get_uv());
    }

    Mesh mesh;
    AtlasMeshData source;
    std::unique_ptr<ParaQuadCutting> cutting;
    std::unique_ptr<PolySquareDeformation> polySquare;
    std::map<int, int> boundaryVk;
    std::string inputPath;
    std::string inputDirectory;
    std::string inputFile;
    std::string phase;
    Stage stage = Stage::Empty;
    int stepIndex = 0;
    double inputDistortion = 0.0;
    double kernelWidth = 4.0;
    double energyExponent = 1.0;
    double splitThreshold = 0.02;
    double chartGap = 6.0;
    double packingEfficiencyBound = 0.8;
};

AtlasPipeline::AtlasPipeline() : impl_(std::make_unique<Impl>()) {}
AtlasPipeline::~AtlasPipeline() = default;

void AtlasPipeline::initialize(const std::string& inputPath)
{
    impl_->initialize(inputPath);
}

void AtlasPipeline::executeNextStep(const ProgressCallback& progressCallback)
{
    impl_->executeNextStep(progressCallback);
}

bool AtlasPipeline::isComplete() const
{
    return impl_->isComplete();
}

std::string AtlasPipeline::nextStepName() const
{
    return impl_->nextStepName();
}

AtlasPipelineSnapshot AtlasPipeline::snapshot() const
{
    return impl_->snapshot();
}
