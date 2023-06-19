
layout(location = 0) out vec4 color;

// int raycast_value(int i, out float value)
// {
//     pnanovdb_coord_t bbox_min = pnanovdb_root_get_bbox_min(vdb[i].buf, vdb[i].accessor.root);
//     pnanovdb_coord_t bbox_max = pnanovdb_root_get_bbox_max(vdb[i].buf, vdb[i].accessor.root);
//     pnanovdb_vec3_t bbox_minf = pnanovdb_coord_to_vec3(bbox_min);
//     pnanovdb_vec3_t bbox_maxf = pnanovdb_coord_to_vec3(pnanovdb_coord_add(bbox_max, pnanovdb_coord_uniform(1)));

//     vec3 origin = get_origin();
//     vec3 dir = get_dir();
//     float tmin = 0.1;
//     float tmax = 1e5;
//     float thit = 0;
//     float v = 0;

//     pnanovdb_bool_t hit = pnanovdb_hdda_ray_clip(PNANOVDB_REF(bbox_minf), PNANOVDB_REF(bbox_maxf), origin, PNANOVDB_REF(tmin), dir, PNANOVDB_REF(tmax));

//     if (!hit || tmax > 1.0e20f || tmax < 0)
//     {
//         return 0;
//     }

//     pnanovdb_vec3_t pos = pnanovdb_hdda_ray_start(origin, tmin, dir);
//     pnanovdb_coord_t ijk = pnanovdb_hdda_pos_to_ijk(PNANOVDB_REF(pos));

//     uint level;
//     pnanovdb_address_t address = pnanovdb_readaccessor_get_value_address_and_level(vdb[i].grid_type, vdb[i].buf, vdb[i].accessor, PNANOVDB_REF(ijk), PNANOVDB_REF(level));
//     float v0 = pnanovdb_root_read_float_typed(vdb[i].grid_type, vdb[i].buf, address, ijk, level);

//     pnanovdb_int32_t dim = pnanovdb_uint32_as_int32(pnanovdb_readaccessor_get_dim(vdb[i].grid_type, vdb[i].buf, vdb[i].accessor, PNANOVDB_REF(ijk)));
//     pnanovdb_hdda_t hdda;
//     pnanovdb_hdda_init(PNANOVDB_REF(hdda), origin, tmin, dir, tmax, dim);

//     do
//     {
//         pnanovdb_vec3_t pos_start = pnanovdb_hdda_ray_start(origin, hdda.tmin + 0.001f, dir);
//         ijk = pnanovdb_hdda_pos_to_ijk(PNANOVDB_REF(pos_start));
//         dim = pnanovdb_uint32_as_int32(pnanovdb_readaccessor_get_dim(vdb[i].grid_type, vdb[i].buf, vdb[i].accessor,
//         PNANOVDB_REF(ijk))); pnanovdb_hdda_update(PNANOVDB_REF(hdda), origin, dir, dim);

//         if (hdda.dim > 1 || !pnanovdb_readaccessor_is_active(vdb[i].grid_type, vdb[i].buf, vdb[i].accessor, PNANOVDB_REF(ijk)))
//         {
//             continue;
//         }

//         do
//         {
//             ijk = hdda.voxel;
//             pnanovdb_address_t address =
//                 pnanovdb_readaccessor_get_value_address_and_level(vdb[i].grid_type, vdb[i].buf, vdb[i].accessor, PNANOVDB_REF(ijk), level);
//             PNANOVDB_DEREF(v) = pnanovdb_root_read_float_typed(vdb[i].grid_type, vdb[i].buf, address, ijk, level);

//             if (PNANOVDB_DEREF(v) > 0.0f)
//             {
//                 PNANOVDB_DEREF(thit) = hdda.tmin;
//                 value = v;
//                 return 2;
//             }
//         } while (pnanovdb_hdda_step(PNANOVDB_REF(hdda)) &&
//                  pnanovdb_readaccessor_is_active(vdb[i].grid_type, vdb[i].buf, vdb[i].accessor, PNANOVDB_REF(hdda.voxel)));
//     } while (pnanovdb_hdda_step(PNANOVDB_REF(hdda)));

//     return 1;
// }

float dust_ss_stage_2(vec3 origin, vec3 dir)
{
    return 0.1;
    pnanovdb_coord_t bbox_min = pnanovdb_root_get_bbox_min(vdb[0].buf, vdb[0].accessor.root);
    pnanovdb_coord_t bbox_max = pnanovdb_root_get_bbox_max(vdb[0].buf, vdb[0].accessor.root);
    pnanovdb_vec3_t bbox_minf = pnanovdb_coord_to_vec3(bbox_min);
    pnanovdb_vec3_t bbox_maxf = pnanovdb_coord_to_vec3(pnanovdb_coord_add(bbox_max, pnanovdb_coord_uniform(1)));

    float tmin = 0.1;
    float tmax = 1e5;
    float thit = 0;
    float acc_density = 0.0;
    float threshold = 1.0;

    pnanovdb_bool_t hit = pnanovdb_hdda_ray_clip(PNANOVDB_REF(bbox_minf), PNANOVDB_REF(bbox_maxf), origin, PNANOVDB_REF(tmin), dir, PNANOVDB_REF(tmax));

    if (!hit || tmax > 1.0e20f || tmax < 0)
    {
        return 0;
    }

    pnanovdb_vec3_t pos = pnanovdb_hdda_ray_start(origin, tmin, dir);
    pnanovdb_coord_t ijk = pnanovdb_hdda_pos_to_ijk(PNANOVDB_REF(pos));

    uint level;
    pnanovdb_address_t address = pnanovdb_readaccessor_get_value_address_and_level(vdb[0].grid_type, vdb[0].buf, vdb[0].accessor, PNANOVDB_REF(ijk), PNANOVDB_REF(level));
    float v0 = pnanovdb_root_read_float_typed(vdb[0].grid_type, vdb[0].buf, address, ijk, level);

    pnanovdb_int32_t dim = pnanovdb_uint32_as_int32(pnanovdb_readaccessor_get_dim(vdb[0].grid_type, vdb[0].buf, vdb[0].accessor, PNANOVDB_REF(ijk)));
    pnanovdb_hdda_t hdda;
    pnanovdb_hdda_init(PNANOVDB_REF(hdda), origin, tmin, dir, tmax, dim);

    float saved_t = -1;
    float last_v = -1;

    do
    {
        pnanovdb_vec3_t pos_start = pnanovdb_hdda_ray_start(origin, hdda.tmin + 0.001f, dir);
        ijk = pnanovdb_hdda_pos_to_ijk(PNANOVDB_REF(pos_start));
        dim = pnanovdb_uint32_as_int32(pnanovdb_readaccessor_get_dim(vdb[0].grid_type, vdb[0].buf, vdb[0].accessor, PNANOVDB_REF(ijk)));
        pnanovdb_hdda_update(PNANOVDB_REF(hdda), origin, dir, dim);

        if (hdda.dim > 1 || !pnanovdb_readaccessor_is_active(vdb[0].grid_type, vdb[0].buf, vdb[0].accessor, PNANOVDB_REF(ijk)))
        {
            saved_t = -1;
            last_v = 1;
            continue;
        }

        do
        {
            pnanovdb_address_t address = pnanovdb_readaccessor_get_value_address_and_level(vdb[0].grid_type, vdb[0].buf, vdb[0].accessor, PNANOVDB_REF(hdda.voxel), PNANOVDB_REF(level));

            float v = pnanovdb_root_read_float_typed(vdb[0].grid_type, vdb[0].buf, address, hdda.voxel, level);

            if (saved_t != -1)
            {
                float mass = last_v * (hdda.tmin - saved_t);
                acc_density += mass;

                if (acc_density >= threshold)
                {
                    return threshold;
                }
            }

            saved_t = hdda.tmin;
            last_v = v;
        } while (pnanovdb_hdda_step(PNANOVDB_REF(hdda)));

        if (saved_t != -1)
        {
            float mass = last_v * (hdda.tmin - saved_t);
            acc_density += mass;

            if (acc_density >= threshold)
            {
                return threshold;
            }
        }

        pnanovdb_hdda_init(PNANOVDB_REF(hdda), origin, hdda.tmin + 5.401 * acc_density, dir, tmax, dim);
    } while (pnanovdb_hdda_step(PNANOVDB_REF(hdda)));

    return (clamp(acc_density, 0, threshold));
}

#define vdb_number 1

vec4 dust_ss_stage_1(vec3 origin, vec3 dir, vec3 sun_dir, vec3 sun_col)
{
    pnanovdb_coord_t bbox_min = pnanovdb_root_get_bbox_min(vdb[vdb_number].buf, vdb[vdb_number].accessor.root);
    pnanovdb_coord_t bbox_max = pnanovdb_root_get_bbox_max(vdb[vdb_number].buf, vdb[vdb_number].accessor.root);
    pnanovdb_vec3_t bbox_minf = pnanovdb_coord_to_vec3(bbox_min);
    pnanovdb_vec3_t bbox_maxf = pnanovdb_coord_to_vec3(pnanovdb_coord_add(bbox_max, pnanovdb_coord_uniform(1)));

    float tmin = 0.1;
    float tmax = 1e5;
    float thit = 0;
    float acc_density = 0;
    float acc_light = 0;

    pnanovdb_bool_t hit = pnanovdb_hdda_ray_clip(PNANOVDB_REF(bbox_minf), PNANOVDB_REF(bbox_maxf), origin, PNANOVDB_REF(tmin), dir, PNANOVDB_REF(tmax));

    if (!hit || tmax > 1.0e20f || tmax < 0)
    {
        return vec4(0);
    }

    pnanovdb_vec3_t pos = pnanovdb_hdda_ray_start(origin, tmin, dir);
    pnanovdb_coord_t ijk = pnanovdb_hdda_pos_to_ijk(PNANOVDB_REF(pos));

    uint level;
    pnanovdb_address_t address = pnanovdb_readaccessor_get_value_address_and_level(vdb[vdb_number].grid_type, vdb[vdb_number].buf, vdb[vdb_number].accessor, PNANOVDB_REF(ijk), PNANOVDB_REF(level));
    float v0 = pnanovdb_root_read_float_typed(vdb[vdb_number].grid_type, vdb[vdb_number].buf, address, ijk, level);

    pnanovdb_int32_t dim = pnanovdb_uint32_as_int32(pnanovdb_readaccessor_get_dim(vdb[vdb_number].grid_type, vdb[vdb_number].buf, vdb[vdb_number].accessor, PNANOVDB_REF(ijk)));
    pnanovdb_hdda_t hdda;
    pnanovdb_hdda_init(PNANOVDB_REF(hdda), origin, tmin, dir, tmax, dim);

    float saved_t = -1.0;
    float last_v = -1.0;
    float last_l = -1.0;
    float T = 1.0;
    float threshold = 0.05;
    float mass_multiplier = 10.0;

    float step_length = 0.5;

    while (hdda.tmin < hdda.tmax)
    {  
        hdda.tmin += step_length;
        pnanovdb_vec3_t pos = pnanovdb_hdda_ray_start(origin, hdda.tmin, dir); 

        ijk = pnanovdb_hdda_pos_to_ijk(PNANOVDB_REF(pos));
        pnanovdb_address_t address = pnanovdb_readaccessor_get_value_address_and_level(vdb[vdb_number].grid_type, vdb[vdb_number].buf, vdb[vdb_number].accessor, PNANOVDB_REF(ijk), PNANOVDB_REF(level));

        if (level == 4)
        {
            break;
        }

        if (!pnanovdb_readaccessor_is_active(vdb[vdb_number].grid_type, vdb[vdb_number].buf, vdb[vdb_number].accessor, ijk))
        {
            continue;
        }

        float v = pnanovdb_root_read_float_typed(vdb[vdb_number].grid_type, vdb[vdb_number].buf, address, ijk, level);

        // acc_density += v * 0.05;

        // pnanovdb_vec3_t pos_start = pnanovdb_hdda_ray_start(origin, hdda.tmin + 0.001f, dir);
        // ijk = pnanovdb_hdda_pos_to_ijk(PNANOVDB_REF(pos_start));
        // dim = pnanovdb_uint32_as_int32(pnanovdb_readaccessor_get_dim(vdb[vdb_number].grid_type, vdb[vdb_number].buf, vdb[vdb_number].accessor, PNANOVDB_REF(ijk)));
        // pnanovdb_hdda_update(PNANOVDB_REF(hdda), origin, dir, dim);

        // if (hdda.dim > 1 || !pnanovdb_readaccessor_is_active(vdb[vdb_number].grid_type, vdb[vdb_number].buf, vdb[vdb_number].accessor, PNANOVDB_REF(ijk)))
        // {
        //     saved_t = -1.0;
        //     last_v = 0.0;
        //     last_l = 0.0;
        //     continue;
        // }

        // do
        // {
        //     pnanovdb_address_t address = pnanovdb_readaccessor_get_value_address_and_level(vdb[vdb_number].grid_type, vdb[vdb_number].buf, vdb[0].accessor, PNANOVDB_REF(hdda.voxel), PNANOVDB_REF(level));

        //     // float v = pnanovdb_root_read_float_typed(vdb[0].grid_type, vdb[0].buf, address, hdda.voxel, level);
        //     float v = vdb_read_world_value_0(origin + dir * hdda.tmin);
        //     float l = 0;

        //     if (v > 0)
        //     {
        //         l = v;
        //     }

        //     if (saved_t != -1)
        //     {
        //         float step_length = hdda.tmin - saved_t;
        //         float mass = last_v * step_length * mass_multiplier;
        //         float weight = exp(-mass);

                acc_density += T * (v * (1.0 - 0.9));

                T *= 1.0 - v * 0.1;

        //         if (T < threshold)
        //         {
        //             return vec4(vec3(acc_density), 1.0);
        //         }
        //     }

        //     saved_t = hdda.tmin;
        //     last_v = v;
        //     last_l = l;

        //     pnanovdb_hdda_init(PNANOVDB_REF(hdda), origin, hdda.tmin + 0.66, dir, tmax, dim);
        // } while (pnanovdb_hdda_step(PNANOVDB_REF(hdda)));

        // if (saved_t != -1)
        // {
        //     acc_density += last_v * (hdda.tmin - saved_t);

        //     if (T < threshold)
        //     {
        //         return vec4(vec3(acc_density), 1.0);
        //     }
        // }

    }

    return vec4(vec3(acc_density), 1.0 - T);
}

void main()
{
    vdb_init_1();

    vec3 origin = get_origin();
    vec3 start_dir = get_dir();
    vec3 sun_dir = normalize(vec3(1.0, -1.0, 1.0));
    vec3 sun_col = vec3(1.0);

    color = dust_ss_stage_1(origin, start_dir, sun_dir, sun_col);
    color.a = 1.0;
}
