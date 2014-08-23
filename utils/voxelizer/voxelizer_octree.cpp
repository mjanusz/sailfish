#include <algorithm>
#include <cstdio>
#include <fstream>
#include <functional>
#include <iomanip>

#include <cvmlcpp/base/Matrix>
#include <cvmlcpp/math/Euclid>
#include <cvmlcpp/volume/Geometry>
#include <cvmlcpp/volume/VolumeIO>
#include <cvmlcpp/volume/Voxelizer>
#include <cvmlcpp/volume/VoxelTools>

#include "io.hpp"
#include "subdomain.hpp"

using namespace cvmlcpp;
using namespace std;

void FindFluidExtent(const Octree::DNode& node, const int max_depth, iPoint3D* pmin, iPoint3D* pmax) {
    if (!node.isLeaf()) {
        for (int i = 0; i < Octree::N; i++) {
            FindFluidExtent(node[i], max_depth, pmin, pmax);
        }
        return;
    }

    if (node() == kFluid) {
        const auto loc = NodeLocation(node, max_depth);
        const auto ext = NodeExtent(node, max_depth);

        if (loc.x() < pmin->x()) pmin->x() = loc.x();
        if (loc.y() < pmin->y()) pmin->y() = loc.y();
        if (loc.z() < pmin->z()) pmin->z() = loc.z();

        if (ext.x() > pmax->x()) pmax->x() = ext.x();
        if (ext.y() > pmax->y()) pmax->y() = ext.y();
        if (ext.z() > pmax->z()) pmax->z() = ext.z();
    }
}

void OctreeToMatrix(const Octree::DNode& node,
        const int max_depth,
        const iPoint3D& pmin,
        const iPoint3D& pmax,
        const int pad,
        Matrix<char, 3u> *mtx_ptr) {
    auto& mtx = *mtx_ptr;
    // Prepare the array first.
    if (node.depth() == 0) {
        std::array<int, 3> size = {pmax.x() - pmin.x() + 1 + 2 * pad,
                                   pmax.y() - pmin.y() + 1 + 2 * pad,
                                   pmax.z() - pmin.z() + 1 + 2 * pad};
        mtx.resize(size.data());
        std::fill(mtx.begin(), mtx.end(), kWall);
    }
    if (!node.isLeaf()) {
        for (int i = 0; i < Octree::N; i++) {
            OctreeToMatrix(node[i], max_depth, pmin, pmax, pad, mtx_ptr);
        }
        return;
    }

    // The array is already filled with wall nodes -- nothing to do.
    if (node() != kFluid) return;

    const auto loc = NodeLocation(node, max_depth);
    const auto ext = NodeExtent(node, max_depth);

    for (size_t x = loc.x() - pmin.x() + pad; x <= ext.x() - pmin.x() + pad; x++) {
        for (size_t y = loc.y() - pmin.y() + pad; y <= ext.y() - pmin.y() + pad; y++) {
            for (size_t z = loc.z() - pmin.z() + pad; z <= ext.z() - pmin.z() + pad; z++) {
                mtx[x][y][z] = kFluid;
            }
        }
    }
}

int ilog2(int v) {
    int ret = 0;
    while (v) {
        ret++;
        v >>= 1;
    }
    return ret;
}

int main(int argc, char **argv)
{
    Geometry<float> geometry;
    int num_voxels = 100;

    if (argc < 2) {
        cerr << "Usage: ./voxelizer <STL file> [num_voxels]" << endl;
        return -1;
    }

    if (argc >= 3) {
        num_voxels = atoi(argv[2]);
    }

    readSTL(geometry, argv[1]);

    std::cout << "Bounding box: "
           << geometry.max(0) - geometry.min(0) << " "
           << geometry.max(1) - geometry.min(1) << " "
           << geometry.max(2) - geometry.min(2) << std::endl;

    Octree octree(kWall);

    double max_span = 0.0;
    for (std::size_t d = 0; d < 3; ++d) {
        max_span = std::max(max_span, static_cast<double>(geometry.max(d)) -
                                      static_cast<double>(geometry.min(d)));
    }
    const double voxel_size = max_span / num_voxels;
    std::cout << "Voxel size: " << voxel_size << endl;

    voxelize(geometry, octree, voxel_size, kFluid /* inside */, kWall /* outside */);
    cout << "Tree depth: actual: " << octree.max_depth() << " want: " << ilog2(num_voxels) << endl;

    iPoint3D pmin(10000, 10000, 10000);
    iPoint3D pmax(0, 0, 0);
    int md = ilog2(num_voxels);
    FindFluidExtent(octree.root(), md, &pmin, &pmax);
    cout << "Bounding box: " << pmin << " - " << pmax << endl;

    // At this point we could use expand(octree, voxels), but this is inefficent
    // for domains that are not cubes. Instead, we use the custom implementation
    // that only fills the data from [pmin, pmax].
    Matrix<char, 3u> voxels;
    OctreeToMatrix(octree, md, pmin, pmax, 1, &voxels);
	const std::size_t *ext = voxels.extents();
    cout << "Array size: " << ext[0] << ", " << ext[1] << ", " << ext[2] << endl;
    SaveAsNumpy(voxels, "test.npy");
    return 0;
#if 0
    auto subs = ToSubdomains(octree.root());
    int total_vol = 0;
    int fluid_vol = 0;

    for (auto& s : subs) {
        cout << s.JSON() << " " << s.volume() << " " << s.fill_fraction() << endl;
        total_vol += s.volume();
        fluid_vol += s.fluid_nodes();
    }

    cout << total_vol << " " << fluid_vol << " " << static_cast<double>(fluid_vol) / total_vol << endl;

    return 0;
#endif
}
