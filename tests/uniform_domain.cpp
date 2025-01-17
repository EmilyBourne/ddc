// SPDX-License-Identifier: MIT

#include <cstddef>

#include <ddc/ddc.hpp>

#include <gtest/gtest.h>

namespace {

struct X;
struct Y;

using DDimX = ddc::UniformPointSampling<X>;
using DDimY = ddc::UniformPointSampling<Y>;
using NUDDimX = ddc::NonUniformPointSampling<X>;
using NUDDimY = ddc::NonUniformPointSampling<Y>;

using DDomNull = ddc::DiscreteDomain<std::nullptr_t>;
using DDomX = ddc::DiscreteDomain<DDimX>;
using NUDDomX = ddc::DiscreteDomain<NUDDimX>;
using DDomXY = ddc::DiscreteDomain<DDimX, DDimY>;
using NUDDomXY = ddc::DiscreteDomain<NUDDimX, NUDDimY>;
using DDomXNUDDomY = ddc::DiscreteDomain<DDimX, NUDDimY>;

} // namespace

TEST(UniformDomainTest, NonDiscreteDomainSpecializationValue)
{
    EXPECT_FALSE(ddc::is_uniform_domain_v<std::nullptr_t>);
    EXPECT_FALSE(ddc::is_uniform_domain_v<X>);
}

TEST(UniformDomainTest, DiscreteDomainSpecializationValue)
{
    EXPECT_FALSE(ddc::is_uniform_domain_v<DDomNull>);

    EXPECT_TRUE(ddc::is_uniform_domain_v<DDomX>);
    EXPECT_TRUE(ddc::is_uniform_domain_v<DDomXY>);

    EXPECT_FALSE(ddc::is_uniform_domain_v<NUDDomX>);
    EXPECT_FALSE(ddc::is_uniform_domain_v<NUDDomXY>);
    EXPECT_FALSE(ddc::is_uniform_domain_v<DDomXNUDDomY>);
}
