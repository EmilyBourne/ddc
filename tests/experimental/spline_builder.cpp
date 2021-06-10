#include <array>
#include <cstdint>
#include <type_traits>

#include <gtest/gtest.h>

#include "experimental/block_spline.h"
#include "experimental/bsplines_uniform.h"
#include "experimental/spline_builder.hpp"
#include "experimental/spline_evaluator.hpp"

#include "block_nonuniform.h"
#include "mdomain.h"
#include "null_boundary_value.h"

struct DimX
{
    static constexpr bool PERIODIC = true;
};
using UniformMDomainX = UniformMDomain<DimX>;
using RCoordX = RCoord<DimX>;
using MCoordX = MCoord<DimX>;

class PolynomialEvaluator
{
private:
    std::vector<double> m_poly_coeff;

public:
    PolynomialEvaluator() = default;

    PolynomialEvaluator(std::vector<double> const& poly_coef) : m_poly_coeff(poly_coef) {}

    double operator()(double const x) const noexcept
    {
        return eval(x, 0);
    }

    template <class Domain>
    void operator()(BlockView<Domain, double>& block_mesh) const
    {
        auto const& domain = block_mesh.domain();

        for (std::size_t i = 0; i < domain.size(); ++i) {
            block_mesh(i) = eval(domain.to_real(domain[i]), 0);
        }
    }

    double deriv(double const x, int const derivative) const noexcept
    {
        return eval(x, derivative);
    }

    template <class Domain>
    void deriv(BlockView<Domain, double>& block_mesh, int const derivative) const
    {
        auto const& domain = block_mesh.domain();

        for (std::size_t i = 0; i < domain.size(); ++i) {
            block_mesh(i) = eval(domain.to_real(domain[i]), derivative);
        }
    }

private:
    double eval(double const x, int const d) const noexcept
    {
        double y = 0.0;
        for (std::size_t i = std::max(d, 0); i < m_poly_coeff.size(); ++i) {
            y += falling_factorial(i, d) * std::pow(x, (i - d)) * m_poly_coeff[i];
        }
        return y;
    }

    constexpr int falling_factorial(int x, int n) const
    {
        double c = 1.;
        if (n >= 0) {
            for (int k = 0; k < n; ++k) {
                c *= (x - k);
            }
        } else {
            for (int k = -1; k >= n; --k) {
                c /= (x - k);
            }
        }
        return c;
    }
};

class CosineEvaluator
{
    static inline constexpr double s_two_pi = 2. * M_PI;

private:
    double m_c0 = 1.;

    double m_c1 = 0.;

public:
    CosineEvaluator() = default;

    CosineEvaluator(double c0, double c1) : m_c0(c0), m_c1(c1) {}

    double operator()(double const x) const noexcept
    {
        return eval(x, 0);
    }

    template <class Domain>
    void operator()(BlockView<Domain, double>& block_mesh) const
    {
        auto const& domain = block_mesh.domain();

        for (std::size_t i = 0; i < domain.size(); ++i) {
            block_mesh(i) = eval(domain.to_real(domain[i]), 0);
        }
    }

    double deriv(double const x, int const derivative) const noexcept
    {
        return eval(x, derivative);
    }

    template <class Domain>
    void deriv(BlockView<Domain, double>& block_mesh, int const derivative) const
    {
        auto const& domain = block_mesh.domain();

        for (std::size_t i = 0; i < domain.size(); ++i) {
            block_mesh(i) = eval(domain.to_real(domain[i]), derivative);
        }
    }

private:
    double eval(double const x, int const derivative) const noexcept
    {
        return std::pow(s_two_pi * m_c0, derivative)
               * std::cos(M_PI_2 * derivative + s_two_pi * (m_c0 * x + m_c1));
    }
};

namespace experimental {

TEST(SplineBuilder, Constructor)
{
    UniformMesh<DimX> const mesh(RCoordX(0.), RCoordX(0.02));
    UniformMDomainX const dom(mesh, MCoordX(101));

    std::integral_constant<std::size_t, 2> constexpr spline_degree;
    auto&& bsplines = make_bsplines(dom, spline_degree);

    BoundCond left_bc = DimX::PERIODIC ? BoundCond::PERIODIC : BoundCond::HERMITE;
    BoundCond right_bc = DimX::PERIODIC ? BoundCond::PERIODIC : BoundCond::HERMITE;
    SplineBuilder spline_builder(bsplines, left_bc, right_bc);
    auto&& interpolation_domain = spline_builder.interpolation_domain();
}

TEST(SplineBuilder, BuildSpline)
{
    int constexpr degree = 10;
    using NonUniformMeshX = NonUniformMesh<DimX>;
    using UniformMeshX = UniformMesh<DimX>;
    using UniformBSplinesX2 = UniformBSplines<DimX, degree>;
    using BlockSplineX2 = Block<UniformBSplinesX2, double>;
    using NonUniformDomainX = MDomainImpl<NonUniformMeshX>;
    using BlockNonUniformX = Block<NonUniformDomainX, double>;

    BoundCond constexpr left_bc = DimX::PERIODIC ? BoundCond::PERIODIC : BoundCond::HERMITE;
    BoundCond constexpr right_bc = DimX::PERIODIC ? BoundCond::PERIODIC : BoundCond::HERMITE;
    RCoordX constexpr x0 = 0.;
    RCoordX constexpr xN = 1.;
    MCoordX constexpr ncells = 100;
    MCoordX constexpr origin = 0;
    MCoordX constexpr npoints = ncells + 1;
    RCoordX constexpr dx = (xN - x0) / ncells;

    // 1. Create BSplines
    UniformMDomainX const dom(x0, xN + dx, origin, npoints);
    UniformBSplinesX2 bsplines(dom);

    // 2. Create a Spline represented by a block over BSplines
    // The block is filled with garbage data, we need to initialize it
    BlockSplineX2 coef(bsplines);

    // 3. Create a SplineBuilder over BSplines using some boundary conditions
    SplineBuilder spline_builder(bsplines, left_bc, right_bc);
    NonUniformDomainX const& interpolation_domain = spline_builder.interpolation_domain();

    // 4. Allocate and fill a block over the interpolation domain
    BlockNonUniformX yvals(interpolation_domain);
    CosineEvaluator cosine_evaluator;
    cosine_evaluator(yvals);

    int constexpr shift = degree % 2; // shift = 0 for even order, 1 for odd order
    std::array<double, degree / 2> Sderiv_lhs_data;
    DSpan1D Sderiv_lhs(Sderiv_lhs_data.data(), Sderiv_lhs_data.size());
    for (int ii = 0; ii < Sderiv_lhs.extent(0); ++ii) {
        Sderiv_lhs(ii) = cosine_evaluator.deriv(x0, ii + shift);
    }
    std::array<double, degree / 2> Sderiv_rhs_data;
    DSpan1D Sderiv_rhs(Sderiv_rhs_data.data(), Sderiv_rhs_data.size());
    for (int ii = 0; ii < Sderiv_rhs.extent(0); ++ii) {
        Sderiv_rhs(ii) = cosine_evaluator.deriv(xN, ii + shift);
    }
    DSpan1D* deriv_l(left_bc == BoundCond::HERMITE ? &Sderiv_lhs : nullptr);
    DSpan1D* deriv_r(right_bc == BoundCond::HERMITE ? &Sderiv_rhs : nullptr);

    // 5. Finally build the spline by filling `block_spline`
    spline_builder(coef, yvals, deriv_l, deriv_r);

    // 6. Create a SplineEvaluator to evaluate the spline at any point in the domain of the BSplines
    SplineEvaluator spline(coef, NullBoundaryValue::value, NullBoundaryValue::value);

    BlockNonUniformX spline_eval(interpolation_domain);
    spline(spline_eval);

    BlockNonUniformX spline_eval_deriv(interpolation_domain);
    spline.deriv(spline_eval_deriv);

    // 7. Checking errors
    double max_norm_error = 0.;
    double max_norm_error_diff = 0.;
    for (std::size_t i = 0; i < interpolation_domain.size(); ++i) {
        auto&& x = interpolation_domain.to_real(interpolation_domain[i]);

        // Compute error
        double const error = spline_eval(i) - yvals(i);
        max_norm_error = std::fmax(max_norm_error, std::fabs(error));

        // Compute error
        double const error_deriv = spline_eval_deriv(i) - cosine_evaluator.deriv(x, 1);
        max_norm_error_diff = std::fmax(max_norm_error_diff, std::fabs(error_deriv));
    }
    EXPECT_LE(max_norm_error, 1.0e-12);
}

} // namespace experimental
