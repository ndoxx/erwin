#include "math/convolution.h"
#include "core/core.h"

#include <functional>
#include <cmath>

namespace erwin
{
namespace math
{

constexpr uint32_t k_simpson_subdivisions = 6;

static float integrate_simpson(std::function<float (float)> f, float lb, float ub, uint32_t subintervals)
{
    // * Simpson's rule is more accurate if we subdivide the interval of integration
    float h        = (ub-lb)/subintervals, // width of subintervals
          sum_odd  = 0.0f,                 // sum of odd subinterval contributions
          sum_even = 0.0f,                 // sum of even subinterval contributions
          y0       = f(lb),                // f value at lower bound
          yn       = f(ub);                // f value at upper bound

    // loop to evaluate intermediary sums
    for(int ii=1; ii<subintervals; ++ii)
    {
        float yy = f(lb + ii*h); // evaluate y_ii
        // sum of odd terms go into sum_odd and sum of even terms go into sum_even
        ((ii%2)?sum_odd:sum_even) += yy;
    }

    // h/3*[y0+yn+4*(y1+y3+y5+...+yn-1)+2*(y2+y4+y6+...+yn-2)]
    return h/3.0f*(y0 + yn + 4.0f*sum_odd + 2.0f*sum_even);
}

static float gaussian_distribution(float x, float mu, float sigma)
{
    float d = x - mu;
    float n = 1.0f / (sqrt(2.0f * M_PI) * sigma);
    return exp(-d*d/(2 * sigma * sigma)) * n;
};

SeparableGaussianKernel::SeparableGaussianKernel(uint32_t half_size, float sigma)
{
	init(half_size, sigma);
}

void SeparableGaussianKernel::init(uint32_t _half_size, float _sigma)
{
	W_ASSERT_FMT(_half_size%2==1, "Gaussian kernel half size must be odd, got %u.", _half_size);
	W_ASSERT_FMT(_half_size<=k_max_kernel_coefficients, "Gaussian kernel half size out of bounds, got %u.", _half_size);
	W_ASSERT_FMT(_sigma>0.f, "Gaussian kernel sigma must be strictly positive, got %f.", _sigma);

	half_size = _half_size;

	std::fill(weights, weights+k_max_kernel_coefficients, 0.f);

    // Compute weights by numerical integration of distribution over each kernel tap
    float sum = 0.0f;
    for(uint32_t ii=0; ii<half_size; ++ii)
    {
        float x_lb = ii - 0.5f;
        float x_ub = ii + 0.5f;
        weights[ii] = integrate_simpson([&](float x){ return gaussian_distribution(x, 0.0f, _sigma); }, x_lb, x_ub, k_simpson_subdivisions);
        // First (central) weight is counted once, other elements must be counted twice
        sum += ((ii==0)?1.0f:2.0f) * weights[ii];
    }

    // Renormalize weights to unit
    for(uint32_t ii=0; ii<half_size; ++ii)
        weights[ii] /= sum;
}

#ifdef W_DEBUG
std::ostream& operator<<(std::ostream& stream, const SeparableGaussianKernel& gk)
{
    stream << "[";
    for(uint32_t ii=0; ii<gk.half_size; ++ii)
    {
        stream << gk.weights[ii];
        if(ii!=gk.half_size-1)
            stream << " ";
    }
    stream << "]";
    return stream;
}
#endif

} // namespace math
} // namespace erwin