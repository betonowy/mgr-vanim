
layout(location = 0) out vec4 color;

int vdb_raycast_value(int i, out float value)
{
    pnanovdb_coord_t bbox_min = pnanovdb_root_get_bbox_min(vdb[i].buf, vdb[i].accessor.root);
    pnanovdb_coord_t bbox_max = pnanovdb_root_get_bbox_max(vdb[i].buf, vdb[i].accessor.root);
    pnanovdb_vec3_t bbox_minf = pnanovdb_coord_to_vec3(bbox_min);
    pnanovdb_vec3_t bbox_maxf = pnanovdb_coord_to_vec3(pnanovdb_coord_add(bbox_max, pnanovdb_coord_uniform(1)));

    vec3 origin = get_origin();
    vec3 direction = get_dir();
    float tmin = 0.1;
    float tmax = 1e5;
    float thit = 0;
    float v = 0;

    pnanovdb_bool_t hit = pnanovdb_hdda_ray_clip(PNANOVDB_REF(bbox_minf), PNANOVDB_REF(bbox_maxf), origin, PNANOVDB_REF(tmin), direction, PNANOVDB_REF(tmax));

    if (!hit || tmax > 1.0e20f || tmax < 0)
    {
        return 0;
    }

    pnanovdb_vec3_t pos = pnanovdb_hdda_ray_start(origin, tmin, direction);
    pnanovdb_coord_t ijk = pnanovdb_hdda_pos_to_ijk(PNANOVDB_REF(pos));

    uint level;
    pnanovdb_address_t address = pnanovdb_readaccessor_get_value_address_and_level(vdb[i].grid_type, vdb[i].buf, vdb[i].accessor, PNANOVDB_REF(ijk), PNANOVDB_REF(level));
    float v0 = pnanovdb_root_read_float_typed(vdb[i].grid_type, vdb[i].buf, address, ijk, level);

    pnanovdb_int32_t dim = pnanovdb_uint32_as_int32(pnanovdb_readaccessor_get_dim(vdb[i].grid_type, vdb[i].buf, vdb[i].accessor, PNANOVDB_REF(ijk)));
    pnanovdb_hdda_t hdda;
    pnanovdb_hdda_init(PNANOVDB_REF(hdda), origin, tmin, direction, tmax, dim);

    do
    {
        pnanovdb_vec3_t pos_start = pnanovdb_hdda_ray_start(origin, hdda.tmin + 0.001f, direction);
        ijk = pnanovdb_hdda_pos_to_ijk(PNANOVDB_REF(pos_start));
        dim = pnanovdb_uint32_as_int32(pnanovdb_readaccessor_get_dim(vdb[i].grid_type, vdb[i].buf, vdb[i].accessor, PNANOVDB_REF(ijk)));
        pnanovdb_hdda_update(PNANOVDB_REF(hdda), origin, direction, dim);

        if (hdda.dim > 1 || !pnanovdb_readaccessor_is_active(vdb[i].grid_type, vdb[i].buf, vdb[i].accessor, PNANOVDB_REF(ijk)))
        {
            continue;
        }

        do
        {
            ijk = hdda.voxel;
            pnanovdb_address_t address =
                pnanovdb_readaccessor_get_value_address_and_level(vdb[i].grid_type, vdb[i].buf, vdb[i].accessor, PNANOVDB_REF(ijk), level);
            PNANOVDB_DEREF(v) = pnanovdb_root_read_float_typed(vdb[i].grid_type, vdb[i].buf, address, ijk, level);

            if (PNANOVDB_DEREF(v) > 0.0f)
            {
                PNANOVDB_DEREF(thit) = hdda.tmin;
                value = v;
                return 2;
            }
        } while (pnanovdb_hdda_step(PNANOVDB_REF(hdda)) &&
                 pnanovdb_readaccessor_is_active(vdb[i].grid_type, vdb[i].buf, vdb[i].accessor, PNANOVDB_REF(hdda.voxel)));
    } while (pnanovdb_hdda_step(PNANOVDB_REF(hdda)));

    return 1;
}

void main()
{
    vdb_init_debug();

    float value_0, value_1;
    int hit_0 = 0, hit_1 = 0;

    if (vdb[0].enabled)
    {
        hit_0 = vdb_raycast_value(0, value_0);
    }

    if (vdb[1].enabled)
    {
        hit_1 = vdb_raycast_value(1, value_1);
    }

    color = vec4(0, 0, 0, 1);

    const float value_multiplier = 33;

    if (hit_0 != 0)
    {
        value_0 *= value_multiplier;
        color.r += value_0 / (value_0 + 1);
        color.b += value_0 / (value_0 + 1);
    }

    if (hit_1 != 0)
    {
        value_1 *= value_multiplier;
        color.g += value_1 / (value_1 + 1);
    }
}
