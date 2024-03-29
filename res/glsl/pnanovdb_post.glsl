layout(std140) uniform world_data
{
    vec3 camera_forward;
    vec3 camera_right;
    vec3 camera_up;
    vec3 camera_origin;
    uvec4 vdb_grid_offsets;
};

in vec2 position;

struct vdb_type
{
    pnanovdb_buf_t buf;
    pnanovdb_grid_handle_t grid_handle;
    pnanovdb_grid_type_t grid_type;
    pnanovdb_tree_handle_t tree_handle;
    pnanovdb_root_handle_t root_handle;
    pnanovdb_readaccessor_t accessor;
    bool enabled;
} vdb[2];

void vdb_init_debug()
{
    vdb[0].grid_handle.address.byte_offset = vdb_grid_offsets[0];

    if (vdb[0].enabled = vdb[0].grid_handle.address.byte_offset != 0xffffffff)
    {
        vdb[0].tree_handle = pnanovdb_grid_get_tree(vdb[0].buf, vdb[0].grid_handle);
        vdb[0].root_handle = pnanovdb_tree_get_root(vdb[0].buf, vdb[0].tree_handle);
        pnanovdb_readaccessor_init(vdb[0].accessor, vdb[0].root_handle);
        vdb[0].grid_type = pnanovdb_grid_get_grid_type(vdb[0].buf, vdb[0].grid_handle);
    }

    vdb[1].grid_handle.address.byte_offset = vdb_grid_offsets[1];

    if (vdb[1].enabled = vdb[1].grid_handle.address.byte_offset != 0xffffffff)
    {
        vdb[1].tree_handle = pnanovdb_grid_get_tree(vdb[1].buf, vdb[1].grid_handle);
        vdb[1].root_handle = pnanovdb_tree_get_root(vdb[1].buf, vdb[1].tree_handle);
        pnanovdb_readaccessor_init(vdb[1].accessor, vdb[1].root_handle);
        vdb[1].grid_type = pnanovdb_grid_get_grid_type(vdb[1].buf, vdb[1].grid_handle);
    }
}

void vdb_init_0()
{
    vdb[0].grid_handle.address.byte_offset = vdb_grid_offsets[0];
    vdb[0].tree_handle = pnanovdb_grid_get_tree(vdb[0].buf, vdb[0].grid_handle);
    vdb[0].root_handle = pnanovdb_tree_get_root(vdb[0].buf, vdb[0].tree_handle);
    pnanovdb_readaccessor_init(vdb[0].accessor, vdb[0].root_handle);
    vdb[0].grid_type = pnanovdb_grid_get_grid_type(vdb[0].buf, vdb[0].grid_handle);
}

void vdb_init_1()
{
    vdb[1].grid_handle.address.byte_offset = vdb_grid_offsets[1];
    vdb[1].tree_handle = pnanovdb_grid_get_tree(vdb[1].buf, vdb[1].grid_handle);
    vdb[1].root_handle = pnanovdb_tree_get_root(vdb[1].buf, vdb[1].tree_handle);
    pnanovdb_readaccessor_init(vdb[1].accessor, vdb[1].root_handle);
    vdb[1].grid_type = pnanovdb_grid_get_grid_type(vdb[1].buf, vdb[1].grid_handle);
}

PNANOVDB_FORCE_INLINE float pnanovdb_root_read_float_typed(pnanovdb_grid_type_t grid_type, pnanovdb_buf_t buf, pnanovdb_address_t address, PNANOVDB_IN(pnanovdb_coord_t) ijk, pnanovdb_uint32_t level)
{
    float ret;
    if (level == 0 && grid_type != PNANOVDB_GRID_TYPE_FLOAT)
    {
        if (grid_type == PNANOVDB_GRID_TYPE_FPN)
        {
            ret = pnanovdb_leaf_fpn_read_float(buf, address, ijk);
        }
        else
        {
            ret = pnanovdb_leaf_fp_read_float(buf, address, ijk, grid_type - PNANOVDB_GRID_TYPE_FP4 + 2u);
        }
    }
    else
    {
        ret = pnanovdb_read_float(buf, address);
    }
    return ret;
}

vec3 vdb_hdda_to_pos(vec3 origin, vec3 dir, float t)
{
    return origin + dir * t;
}

float vdb_read_world_value_0(vec3 pos)
{
    pnanovdb_coord_t ijk = pnanovdb_hdda_pos_to_ijk(pos); 
    pnanovdb_uint32_t level;
    pnanovdb_address_t address = pnanovdb_readaccessor_get_value_address_and_level(vdb[0].grid_type, vdb[0].buf, vdb[0].accessor, ijk, level);
    return pnanovdb_root_read_float_typed(vdb[0].grid_type, vdb[0].buf, address, ijk, level);
}

in layout(pixel_center_integer) vec4 gl_FragCoord;

vec3 get_dither_offset(int offset)
{
    const int size = 4;
    const int next_coord_offset = size * size;

    int x = int(gl_FragCoord.x) & (size - 1);
    int y = (int(gl_FragCoord.y) & (size - 1)) * size;
    int base = x + y + offset;

    vec3 v;

    v.x = pnanovdb_dither_lut[base + next_coord_offset * 0];
    v.y = pnanovdb_dither_lut[base + next_coord_offset * 1];
    v.z = pnanovdb_dither_lut[base + next_coord_offset * 2];

    return v;
}

vec3 get_dither()
{
    const int next_coord_offset = 128;

    int x = int(gl_FragCoord.x) & 7;
    int y = (int(gl_FragCoord.y) & 7) << 3;
    int base = x + y;

    vec3 v;

    bool enable = true;

    v.x = pnanovdb_dither_lookup(enable, base + next_coord_offset * 0);
    v.y = pnanovdb_dither_lookup(enable, base + next_coord_offset * 1);
    v.z = pnanovdb_dither_lookup(enable, base + next_coord_offset * 2);

    return v;
}

vec3 get_origin()
{
    return camera_origin.xzy * vec3(1, 1, -1);
}

vec3 get_dir()
{
    return normalize(camera_forward + camera_right * position.x - camera_up * position.y).xzy * vec3(1, 1, -1);
}
