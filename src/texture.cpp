#include "texture.h"
#include "CGL/color.h"

#include <cmath>
#include <algorithm>

namespace CGL {

    inline Color lerp(float x, Color c0, Color c1) {
        return (1 - x) * c0 + x * c1;
    }

    Color (Texture::*sample_func) (Vector2D, int);

    Color Texture::sample(const SampleParams &sp) {
        // TODO: Task 6: Fill this in.
        if (sp.psm == P_NEAREST) {
            sample_func = &Texture::sample_nearest;
        } else {
            sample_func = &Texture::sample_bilinear;
        }

        if (sp.lsm == L_LINEAR) {
            float level = get_level(sp);
            auto level_lo = (int) floor(level);
            auto level_hi = (int) ceil(level);
            Color sample_lo = (this->*sample_func)(sp.p_uv, level_lo);
            Color sample_hi = (this->*sample_func)(sp.p_uv, level_hi);
            return lerp(level - level_lo, sample_lo, sample_hi);
        } else {
            return (this->*sample_func)(sp.p_uv, (int) get_level(sp));
        }
    }

    float Texture::get_level(const SampleParams &sp) {
        // TODO: Task 6: Fill this in.
        float du = (float) (sp.p_dx_uv[0] - sp.p_uv[0]) * width;
        float dv = (float) (sp.p_dy_uv[1] - sp.p_uv[1]) * height;
        switch (sp.lsm) {
            case L_ZERO:
                return 0;
            case L_NEAREST:
                return max(0.0f, round(log2(sqrt(du * du + dv * dv))));
            case L_LINEAR:
                return max(0.0f, log2(sqrt(du * du + dv * dv)));
            default:
                return 0;
        }
    }

    Color MipLevel::get_texel(int tx, int ty) {
        if (texels.size() < tx * 3 + ty * width * 3 + 1) {
            cout << "Error in get_texels\n";
            exit(0);
        }
        return Color(&texels[tx * 3 + ty * width * 3]);
    }

    double distance2(double x, double y, double u, double v) {
        return (x - u)*(x - u) + (y - v)*(y - v);
    }

    long long pow2(int b) {
        long long ret = 1;
        long long a = 2;
        for (; b; b/=2, a=a*a) {
            if (b & 1) {
                ret = ret * a;
            }
        }
        return ret;
    }

    Color Texture::sample_nearest(Vector2D uv, int level) {
        // TODO: Task 5: Fill this in.
        auto &mip = mipmap[level];
        uv.x = uv.x * width / pow2(level);
        uv.y = uv.y * height / pow2(level);
        vector <pair<int,int>> v;
        int x1 = floor(uv.x);
        int x2 = ceil(uv.x);
        int y1 = floor(uv.y);
        int y2 = ceil(uv.y);
        v.emplace_back(x1, y1);
        v.emplace_back(x1, y2);
        v.emplace_back(x2, y1);
        v.emplace_back(x2, y2);

        Color ans = Color(0, 0, 0);
        double min_dist = 2e9;
        for (auto &p : v) {
            double dis = distance2(p.first, p.second, uv.x, uv.y);
            if (min_dist > dis) {
                min_dist = dis;
                ans = mip.get_texel(p.first, p.second);
            }
        }
        return ans;
    }

    Color Texture::sample_bilinear(Vector2D uv, int level) {
        // TODO: Task 5: Fill this in.
        auto &mip = mipmap[level];
        uv.x = uv.x * width / pow2(level);
        uv.y = uv.y * height / pow2(level);
        vector <pair<int,int>> v;
        int x1 = floor(uv.x);
        int x2 = ceil(uv.x);
        int y1 = floor(uv.y);
        int y2 = ceil(uv.y);

        Color x1_y1 = mip.get_texel(x1, y1);
        Color x1_y2 = mip.get_texel(x1, y2);
        Color x2_y1 = mip.get_texel(x2, y1);
        Color x2_y2 = mip.get_texel(x2, y2);

        double s = sqrt(distance2(x1, y1, uv.x, y1)) / (x2 - x1);
        double t = sqrt(distance2(x1, y1, x1, uv.y)) / (y2 - y1);

        Color new_color = x1_y1*s*t + x2_y1*(1 - s)*t + x2_y2*(1 - t)*(1 - s) + x1_y2*(1 - t)*s;
        return new_color;
    }



    /****************************************************************************/

    // Helpers

    inline void uint8_to_float(float dst[3], unsigned char *src) {
        uint8_t *src_uint8 = (uint8_t *) src;
        dst[0] = src_uint8[0] / 255.f;
        dst[1] = src_uint8[1] / 255.f;
        dst[2] = src_uint8[2] / 255.f;
    }

    inline void float_to_uint8(unsigned char *dst, float src[3]) {
        uint8_t *dst_uint8 = (uint8_t *) dst;
        dst_uint8[0] = (uint8_t) (255.f * max(0.0f, min(1.0f, src[0])));
        dst_uint8[1] = (uint8_t) (255.f * max(0.0f, min(1.0f, src[1])));
        dst_uint8[2] = (uint8_t) (255.f * max(0.0f, min(1.0f, src[2])));
    }

    void Texture::generate_mips(int startLevel) {

        // make sure there's a valid texture
        if (startLevel >= mipmap.size()) {
            std::cerr << "Invalid start level";
        }

        // allocate sublevels
        int baseWidth = mipmap[startLevel].width;
        int baseHeight = mipmap[startLevel].height;
        int numSubLevels = (int) (log2f((float) max(baseWidth, baseHeight)));

        numSubLevels = min(numSubLevels, kMaxMipLevels - startLevel - 1);
        mipmap.resize(startLevel + numSubLevels + 1);

        int width = baseWidth;
        int height = baseHeight;
        for (int i = 1; i <= numSubLevels; i++) {

            MipLevel &level = mipmap[startLevel + i];

            // handle odd size texture by rounding down
            width = max(1, width / 2);
            //assert (width > 0);
            height = max(1, height / 2);
            //assert (height > 0);

            level.width = width;
            level.height = height;
            level.texels = vector<unsigned char>(3 * width * height);
        }

        // create mips
        int subLevels = numSubLevels - (startLevel + 1);
        for (int mipLevel = startLevel + 1; mipLevel < startLevel + subLevels + 1;
             mipLevel++) {

            MipLevel &prevLevel = mipmap[mipLevel - 1];
            MipLevel &currLevel = mipmap[mipLevel];

            int prevLevelPitch = prevLevel.width * 3; // 32 bit RGB
            int currLevelPitch = currLevel.width * 3; // 32 bit RGB

            unsigned char *prevLevelMem;
            unsigned char *currLevelMem;

            currLevelMem = (unsigned char *) &currLevel.texels[0];
            prevLevelMem = (unsigned char *) &prevLevel.texels[0];

            float wDecimal, wNorm, wWeight[3];
            int wSupport;
            float hDecimal, hNorm, hWeight[3];
            int hSupport;

            float result[3];
            float input[3];

            // conditional differentiates no rounding case from round down case
            if (prevLevel.width & 1) {
                wSupport = 3;
                wDecimal = 1.0f / (float) currLevel.width;
            } else {
                wSupport = 2;
                wDecimal = 0.0f;
            }

            // conditional differentiates no rounding case from round down case
            if (prevLevel.height & 1) {
                hSupport = 3;
                hDecimal = 1.0f / (float) currLevel.height;
            } else {
                hSupport = 2;
                hDecimal = 0.0f;
            }

            wNorm = 1.0f / (2.0f + wDecimal);
            hNorm = 1.0f / (2.0f + hDecimal);

            // case 1: reduction only in horizontal size (vertical size is 1)
            if (currLevel.height == prevLevel.height) {
                //assert (currLevel.height == 1);

                for (int i = 0; i < currLevel.width; i++) {
                    wWeight[0] = wNorm * (1.0f - wDecimal * i);
                    wWeight[1] = wNorm * 1.0f;
                    wWeight[2] = wNorm * wDecimal * (i + 1);

                    result[0] = result[1] = result[2] = 0.0f;

                    for (int ii = 0; ii < wSupport; ii++) {
                        uint8_to_float(input, prevLevelMem + 3 * (2 * i + ii));
                        result[0] += wWeight[ii] * input[0];
                        result[1] += wWeight[ii] * input[1];
                        result[2] += wWeight[ii] * input[2];
                    }

                    // convert back to format of the texture
                    float_to_uint8(currLevelMem + (3 * i), result);
                }

                // case 2: reduction only in vertical size (horizontal size is 1)
            } else if (currLevel.width == prevLevel.width) {
                //assert (currLevel.width == 1);

                for (int j = 0; j < currLevel.height; j++) {
                    hWeight[0] = hNorm * (1.0f - hDecimal * j);
                    hWeight[1] = hNorm;
                    hWeight[2] = hNorm * hDecimal * (j + 1);

                    result[0] = result[1] = result[2] = 0.0f;
                    for (int jj = 0; jj < hSupport; jj++) {
                        uint8_to_float(input, prevLevelMem + prevLevelPitch * (2 * j + jj));
                        result[0] += hWeight[jj] * input[0];
                        result[1] += hWeight[jj] * input[1];
                        result[2] += hWeight[jj] * input[2];
                    }

                    // convert back to format of the texture
                    float_to_uint8(currLevelMem + (currLevelPitch * j), result);
                }

                // case 3: reduction in both horizontal and vertical size
            } else {

                for (int j = 0; j < currLevel.height; j++) {
                    hWeight[0] = hNorm * (1.0f - hDecimal * j);
                    hWeight[1] = hNorm;
                    hWeight[2] = hNorm * hDecimal * (j + 1);

                    for (int i = 0; i < currLevel.width; i++) {
                        wWeight[0] = wNorm * (1.0f - wDecimal * i);
                        wWeight[1] = wNorm * 1.0f;
                        wWeight[2] = wNorm * wDecimal * (i + 1);

                        result[0] = result[1] = result[2] = 0.0f;

                        // convolve source image with a trapezoidal filter.
                        // in the case of no rounding this is just a box filter of width 2.
                        // in the general case, the support region is 3x3.
                        for (int jj = 0; jj < hSupport; jj++)
                            for (int ii = 0; ii < wSupport; ii++) {
                                float weight = hWeight[jj] * wWeight[ii];
                                uint8_to_float(input, prevLevelMem +
                                                      prevLevelPitch * (2 * j + jj) +
                                                      3 * (2 * i + ii));
                                result[0] += weight * input[0];
                                result[1] += weight * input[1];
                                result[2] += weight * input[2];
                            }

                        // convert back to format of the texture
                        float_to_uint8(currLevelMem + currLevelPitch * j + 3 * i, result);
                    }
                }
            }
        }
    }

}
