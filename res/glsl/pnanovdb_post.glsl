layout(std140) uniform world_data
{
    vec3 camera_forward;
    vec3 camera_right;
    vec3 camera_up;
    vec3 camera_origin;
    uvec4 vdb_grid_offsets;
};

in vec2 position;

vec3 get_origin()
{
    return camera_origin.xzy * vec3(1, 1, -1);
}

vec3 get_dir()
{
    return normalize(camera_forward + camera_right * position.x - camera_up * position.y).xzy * vec3(1, 1, -1);
}

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

void vdb_init()
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
        ;
    }
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
