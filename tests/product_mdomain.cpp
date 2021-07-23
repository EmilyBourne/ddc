#include <iosfwd>
#include <memory>

#include <gtest/gtest.h>

#include "gtest/gtest_pred_impl.h"

#include "mcoord.h"
#include "non_uniform_mesh.h"
#include "product_mdomain.h"
#include "product_mesh.h"
#include "rcoord.h"
#include "taggedarray.h"
#include "uniform_mesh.h"

#include <experimental/mdspan>

class DimX;
class DimVx;

using MeshX = UniformMesh<DimX>;
using MeshVx = NonUniformMesh<DimVx>;

class ProductMDomainTest : public ::testing::Test
{
protected:
    std::size_t npoints = 11;
    MeshX mesh_x = MeshX(2., 3., npoints);
    MeshVx mesh_vx = MeshVx({-1., 0., 2., 4.});
    ProductMesh<MeshX, MeshVx> mesh_x_vx = ProductMesh(mesh_x, mesh_vx);
    ProductMDomain<MeshX, MeshVx> domain_x_vx = ProductMDomain(
            mesh_x_vx,
            MCoord<MeshX, MeshVx>(0, 0),
            MCoord<MeshX, MeshVx>(npoints - 1, 3));
};

TEST_F(ProductMDomainTest, constructor)
{
    EXPECT_EQ(domain_x_vx.extents(), (MCoord<MeshX, MeshVx>(11, 4)));
    EXPECT_EQ(domain_x_vx.lbound(), (MCoord<MeshX, MeshVx>(0, 0)));
    EXPECT_EQ(domain_x_vx.ubound(), (MCoord<MeshX, MeshVx>(10, 3)));
    EXPECT_EQ(domain_x_vx.size(), 11 * 4);
}

TEST_F(ProductMDomainTest, rmin_rmax)
{
    EXPECT_EQ(domain_x_vx.rmin(), (RCoord<DimX, DimVx>(2., -1.)));
    EXPECT_EQ(domain_x_vx.rmax(), (RCoord<DimX, DimVx>(3., 4.)));
}

TEST_F(ProductMDomainTest, subdomain)
{
    auto subdomain = domain_x_vx.subdomain(1, std::experimental::full_extent);
    EXPECT_EQ(
            subdomain,
            ProductMDomain(ProductMesh(mesh_vx), MCoord<MeshVx>(0), MCoord<MeshVx>(3)));
}