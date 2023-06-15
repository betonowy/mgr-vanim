#include "transform.hpp"

#include <iterator>

namespace dvdb
{
// Big shoutout to https://observablehq.com/@mourner/3d-hilbert-curves
// Saved me so much time. Still, I want to die. NO NO SQUARE.
cube_888<uint16_t> hilbert_curve_indices = {
    0, 64, 65, 1, 9, 73, 72, 8,
    16, 17, 25, 24, 88, 89, 81, 80,
    144, 145, 153, 152, 216, 217, 209, 208,
    200, 192, 128, 136, 137, 129, 193, 201,
    202, 194, 130, 138, 139, 131, 195, 203,
    211, 210, 218, 219, 155, 154, 146, 147,
    83, 82, 90, 91, 27, 26, 18, 19,
    11, 75, 74, 10, 2, 66, 67, 3,
    4, 5, 13, 12, 76, 77, 69, 68,
    132, 140, 204, 196, 197, 205, 141, 133,
    134, 142, 206, 198, 199, 207, 143, 135,
    71, 7, 6, 70, 78, 14, 15, 79,
    87, 23, 22, 86, 94, 30, 31, 95,
    159, 151, 215, 223, 222, 214, 150, 158,
    157, 149, 213, 221, 220, 212, 148, 156,
    92, 93, 85, 84, 20, 21, 29, 28,
    36, 37, 45, 44, 108, 109, 101, 100,
    164, 172, 236, 228, 229, 237, 173, 165,
    166, 174, 238, 230, 231, 239, 175, 167,
    103, 39, 38, 102, 110, 46, 47, 111,
    119, 55, 54, 118, 126, 62, 63, 127,
    191, 183, 247, 255, 254, 246, 182, 190,
    189, 181, 245, 253, 252, 244, 180, 188,
    124, 125, 117, 116, 52, 53, 61, 60,
    59, 51, 115, 123, 122, 114, 50, 58,
    57, 121, 120, 56, 48, 112, 113, 49,
    41, 105, 104, 40, 32, 96, 97, 33,
    34, 35, 43, 42, 106, 107, 99, 98,
    162, 163, 171, 170, 234, 235, 227, 226,
    225, 161, 160, 224, 232, 168, 169, 233,
    241, 177, 176, 240, 248, 184, 185, 249,
    250, 242, 178, 186, 187, 179, 243, 251,
    315, 307, 371, 379, 378, 370, 306, 314,
    313, 377, 376, 312, 304, 368, 369, 305,
    297, 361, 360, 296, 288, 352, 353, 289,
    290, 291, 299, 298, 362, 363, 355, 354,
    418, 419, 427, 426, 490, 491, 483, 482,
    481, 417, 416, 480, 488, 424, 425, 489,
    497, 433, 432, 496, 504, 440, 441, 505,
    506, 498, 434, 442, 443, 435, 499, 507,
    508, 509, 501, 500, 436, 437, 445, 444,
    380, 372, 308, 316, 317, 309, 373, 381,
    382, 374, 310, 318, 319, 311, 375, 383,
    447, 511, 510, 446, 438, 502, 503, 439,
    431, 495, 494, 430, 422, 486, 487, 423,
    359, 367, 303, 295, 294, 302, 366, 358,
    357, 365, 301, 293, 292, 300, 364, 356,
    420, 421, 429, 428, 492, 493, 485, 484,
    476, 477, 469, 468, 404, 405, 413, 412,
    348, 340, 276, 284, 285, 277, 341, 349,
    350, 342, 278, 286, 287, 279, 343, 351,
    415, 479, 478, 414, 406, 470, 471, 407,
    399, 463, 462, 398, 390, 454, 455, 391,
    327, 335, 271, 263, 262, 270, 334, 326,
    325, 333, 269, 261, 260, 268, 332, 324,
    388, 389, 397, 396, 460, 461, 453, 452,
    451, 387, 386, 450, 458, 394, 395, 459,
    467, 466, 474, 475, 411, 410, 402, 403,
    339, 338, 346, 347, 283, 282, 274, 275,
    267, 259, 323, 331, 330, 322, 258, 266,
    265, 257, 321, 329, 328, 320, 256, 264,
    272, 273, 281, 280, 344, 345, 337, 336,
    400, 401, 409, 408, 472, 473, 465, 464,
    456, 392, 393, 457, 449, 385, 384, 448 // keep clang format in check
};

namespace
{
template <typename T>
void carthesian_to_hilbert_impl(const cube_888<T> *src, cube_888<T> *dst)
{
    for (int i = 0; i < std::size(hilbert_curve_indices.values); ++i)
    {
        dst->values[i] = src->values[hilbert_curve_indices.values[i]];
    }
}

template <typename T>
void hilbert_to_carthesian_impl(const cube_888<T> *src, cube_888<T> *dst)
{
    for (int i = 0; i < std::size(hilbert_curve_indices.values); ++i)
    {
        dst->values[hilbert_curve_indices.values[i]] = src->values[i];
    }
}

} // namespace
void transform_carthesian_to_hilbert(const cube_888_f32 *src, cube_888_f32 *dst)
{
    carthesian_to_hilbert_impl(src, dst);
}

void transform_carthesian_to_hilbert(const cube_888_i8 *src, cube_888_i8 *dst)
{
    carthesian_to_hilbert_impl(src, dst);
}

void transform_hilbert_to_carthesian(const cube_888_f32 *src, cube_888_f32 *dst)
{
    hilbert_to_carthesian_impl(src, dst);
}

void transform_hilbert_to_carthesian(const cube_888_i8 *src, cube_888_i8 *dst)
{
    hilbert_to_carthesian_impl(src, dst);
}

// leave for now just in case better analytical transform is found
void transform_init()
{
    for (int i = 0; i < std::size(hilbert_curve_indices.values); ++i)
    {
        hilbert_curve_indices.values[i] = i;
    }

    // std::sort(std::begin(coord_transform_indices.values), std::end(coord_transform_indices.values), [](uint16_t lhs, uint16_t rhs) -> bool {
    //     int lhs_x = (lhs & 0b000000111) >> 0;
    //     int lhs_y = (lhs & 0b000111000) >> 3;
    //     int lhs_z = (lhs & 0b111000000) >> 6;

    //     int rhs_x = (rhs & 0b000000111) >> 0;
    //     int rhs_y = (rhs & 0b000111000) >> 3;
    //     int rhs_z = (rhs & 0b111000000) >> 6;

    //     int lhs_sum = lhs_x * lhs_x + lhs_y * lhs_y + lhs_z * lhs_z;
    //     int rhs_sum = rhs_x * rhs_x + rhs_y * rhs_y + rhs_z * rhs_z;

    //     return (lhs_sum < rhs_sum) || (lhs < rhs);
    // });
}
} // namespace dvdb
