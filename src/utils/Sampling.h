#pragma once
#include "utils/Math.h"

namespace narvalengine {
    struct Distribution1D {
    public:
        std::vector<float> func;
        std::vector<float> cdf;
        float funcInt;
        
        //Takes n values of a piece-wise function f
        Distribution1D (const float* f, int n) {
            func = std::vector<float>(f, f + n);
            cdf = std::vector<float>(n + 1);

            cdf[0] = 0;
            for (int i = 1; i < n + 1; i++) 
                cdf[i] = cdf[i - 1] + func[i - 1] / n;

            // Transform step function integral into CDF
            funcInt = cdf[n];
            if (funcInt == 0)
                for (int i = 1; i < n + 1; i++) 
                    cdf[i] = float(i) / float(n);
            else 
                for (int i = 1; i < n + 1; i++) 
                    cdf[i] /= funcInt;
        }

        int count() {
            return func.size(); 
        }

        float sampleContinuous(float u, float* pdf, int* off = nullptr) {
            // Find surrounding CDF segments and _offset_
            int offset = binarySearch(cdf.size(), [&](int index) { return cdf[index] <= u; });
            
            if (off) 
                *off = offset;

            // Compute offset along CDF segment
            float du = u - cdf[offset];
            if ((cdf[offset + 1] - cdf[offset]) > 0) 
                du /= (cdf[offset + 1] - cdf[offset]);

            // Compute PDF for sampled offset
            if (pdf) *pdf = (funcInt > 0) ? func[offset] / funcInt : 0;

            // Return $x\in{}[0,1)$ corresponding to sample
            return (offset + du) / count();
        }

        int sampleDiscrete(float u, float* pdf = nullptr, float* uRemapped = nullptr) {
            // Find surrounding CDF segments and _offset_
            int offset = binarySearch((int)cdf.size(), [&](int index) { return cdf[index] <= u; });
            
            if (pdf) 
                *pdf = (funcInt > 0) ? func[offset] / (funcInt * count()) : 0;
            
            if (uRemapped)
                *uRemapped = (u - cdf[offset]) / (cdf[offset + 1] - cdf[offset]);
            
            return offset;
        }

        float discretePDF(int index) {
            return func[index] / (funcInt * count());
        }
    };

    class Distribution2D {
    private:
        // Distribution2D Private Data
        std::vector<std::unique_ptr<Distribution1D>> pConditionalV;
        std::unique_ptr<Distribution1D> pMarginal;

    public:
        float* data;
        int width, height;

        //Distribution2D(const Float *data, int nu, int nv);
        Distribution2D(const float* func, int width, int height) {
            pConditionalV.reserve(height);

            for (int v = 0; v < height; ++v) 
                // Compute conditional sampling distribution for $\tilde{v}$
                pConditionalV.emplace_back(new Distribution1D(&func[v * width], height));
            
            // Compute marginal sampling distribution $p[\tilde{v}]$
            std::vector<float> marginalFunc;
            marginalFunc.reserve(height);
            for (int v = 0; v < height; ++v)
                marginalFunc.push_back(pConditionalV[v]->funcInt);

            pMarginal.reset(new Distribution1D(&marginalFunc[0], height));
        }

        glm::vec2 sampleContinuous(const glm::vec2 u, float* pdf) const {
            float pdfs[2];
            int v;
            float d1 = pMarginal->sampleContinuous(u[1], &pdfs[1], &v);
            float d0 = pConditionalV[v]->sampleContinuous(u[0], &pdfs[0]);
            *pdf = pdfs[0] * pdfs[1];
            return glm::vec2(d0, d1);
        }

        float pdf(const glm::vec2 p) const {
            int iu = glm::clamp(int(p[0] * pConditionalV[0]->count()), 0, pConditionalV[0]->count() - 1);
            int iv = glm::clamp(int(p[1] * pMarginal->count()), 0, pMarginal->count() - 1);
            return pConditionalV[iv]->func[iu] / pMarginal->funcInt;
        }
    };
}
