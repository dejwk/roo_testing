if(CONFIG_BT_ENABLED)
    if(CONFIG_IDF_TARGET_ESP32)
        set(srcs "controller/esp32/bt.c"
                 "controller/esp32/hli_api.c"
                 "controller/esp32/hli_vectors.S")
    elseif(CONFIG_IDF_TARGET_ESP32C3)
        set(srcs "controller/esp32c3/bt.c")
    elseif(CONFIG_IDF_TARGET_ESP32S3)
        set(srcs "controller/esp32s3/bt.c")
    endif()

    set(include_dirs common/osi/include)

    if(CONFIG_IDF_TARGET_ESP32)
        list(APPEND include_dirs include/esp32/include)
    elseif(CONFIG_IDF_TARGET_ESP32C3)
        list(APPEND include_dirs include/esp32c3/include)
    elseif(CONFIG_IDF_TARGET_ESP32S3)
        list(APPEND include_dirs include/esp32s3/include)
    endif()

    list(APPEND priv_include_dirs
            common/btc/include
            common/include)
    list(APPEND include_dirs
         common/api/include/api
         common/btc/profile/esp/blufi/include
         common/btc/profile/esp/include)

    list(APPEND srcs "common/btc/core/btc_alarm.c"
         "common/api/esp_blufi_api.c"
         "common/btc/core/btc_manage.c"
         "common/btc/core/btc_task.c"
         "common/btc/profile/esp/blufi/blufi_prf.c"
         "common/btc/profile/esp/blufi/blufi_protocol.c"
         "common/osi/alarm.c"
         "common/osi/allocator.c"
         "common/osi/buffer.c"
         "common/osi/config.c"
         "common/osi/fixed_queue.c"
         "common/osi/future.c"
         "common/osi/hash_functions.c"
         "common/osi/hash_map.c"
         "common/osi/list.c"
         "common/osi/mutex.c"
         "common/osi/thread.c"
         "common/osi/osi.c"
         "common/osi/semaphore.c")

    if(CONFIG_BT_BLUEDROID_ENABLED)

        list(APPEND priv_include_dirs
            host/bluedroid/bta/include
            host/bluedroid/bta/ar/include
            host/bluedroid/bta/av/include
            host/bluedroid/bta/dm/include
            host/bluedroid/bta/gatt/include
            host/bluedroid/bta/hf_ag/include
            host/bluedroid/bta/hf_client/include
            host/bluedroid/bta/hd/include
            host/bluedroid/bta/hh/include
            host/bluedroid/bta/jv/include
            host/bluedroid/bta/sdp/include
            host/bluedroid/bta/sys
            host/bluedroid/device/include
            host/bluedroid/hci/include
            host/bluedroid/external/sbc/decoder/include
            host/bluedroid/external/sbc/encoder/include
            host/bluedroid/external/sbc/plc/include
            host/bluedroid/btc/profile/esp/include
            host/bluedroid/btc/profile/std/a2dp/include
            host/bluedroid/btc/profile/std/include
            host/bluedroid/btc/include
            host/bluedroid/stack/btm/include
            host/bluedroid/stack/gap/include
            host/bluedroid/stack/gatt/include
            host/bluedroid/stack/hid/include
            host/bluedroid/stack/l2cap/include
            host/bluedroid/stack/sdp/include
            host/bluedroid/stack/smp/include
            host/bluedroid/stack/avct/include
            host/bluedroid/stack/avrc/include
            host/bluedroid/stack/avdt/include
            host/bluedroid/stack/a2dp/include
            host/bluedroid/stack/rfcomm/include
            host/bluedroid/stack/include
            host/bluedroid/common/include)

        list(APPEND include_dirs host/bluedroid/api/include/api)

        list(APPEND srcs "host/bluedroid/api/esp_a2dp_api.c"
                   "host/bluedroid/api/esp_avrc_api.c"
                   "host/bluedroid/api/esp_bt_device.c"
                   "host/bluedroid/api/esp_bt_main.c"
                   "host/bluedroid/api/esp_gap_ble_api.c"
                   "host/bluedroid/api/esp_gap_bt_api.c"
                   "host/bluedroid/api/esp_gatt_common_api.c"
                   "host/bluedroid/api/esp_gattc_api.c"
                   "host/bluedroid/api/esp_gatts_api.c"
                   "host/bluedroid/api/esp_hidd_api.c"
                   "host/bluedroid/api/esp_hidh_api.c"
                   "host/bluedroid/api/esp_hf_ag_api.c"
                   "host/bluedroid/api/esp_hf_client_api.c"
                   "host/bluedroid/api/esp_spp_api.c"
                   "host/bluedroid/bta/ar/bta_ar.c"
                   "host/bluedroid/bta/av/bta_av_aact.c"
                   "host/bluedroid/bta/av/bta_av_act.c"
                   "host/bluedroid/bta/av/bta_av_api.c"
                   "host/bluedroid/bta/av/bta_av_cfg.c"
                   "host/bluedroid/bta/av/bta_av_ci.c"
                   "host/bluedroid/bta/av/bta_av_main.c"
                   "host/bluedroid/bta/av/bta_av_sbc.c"
                   "host/bluedroid/bta/av/bta_av_ssm.c"
                   "host/bluedroid/bta/dm/bta_dm_act.c"
                   "host/bluedroid/bta/dm/bta_dm_api.c"
                   "host/bluedroid/bta/dm/bta_dm_cfg.c"
                   "host/bluedroid/bta/dm/bta_dm_ci.c"
                   "host/bluedroid/bta/dm/bta_dm_co.c"
                   "host/bluedroid/bta/dm/bta_dm_main.c"
                   "host/bluedroid/bta/dm/bta_dm_pm.c"
                   "host/bluedroid/bta/dm/bta_dm_sco.c"
                   "host/bluedroid/bta/dm/bta_dm_qos.c"
                   "host/bluedroid/bta/gatt/bta_gatt_common.c"
                   "host/bluedroid/bta/gatt/bta_gattc_act.c"
                   "host/bluedroid/bta/gatt/bta_gattc_api.c"
                   "host/bluedroid/bta/gatt/bta_gattc_cache.c"
                   "host/bluedroid/bta/gatt/bta_gattc_ci.c"
                   "host/bluedroid/bta/gatt/bta_gattc_co.c"
                   "host/bluedroid/bta/gatt/bta_gattc_main.c"
                   "host/bluedroid/bta/gatt/bta_gattc_utils.c"
                   "host/bluedroid/bta/gatt/bta_gatts_act.c"
                   "host/bluedroid/bta/gatt/bta_gatts_api.c"
                   "host/bluedroid/bta/gatt/bta_gatts_co.c"
                   "host/bluedroid/bta/gatt/bta_gatts_main.c"
                   "host/bluedroid/bta/gatt/bta_gatts_utils.c"
                   "host/bluedroid/bta/hd/bta_hd_api.c"
                   "host/bluedroid/bta/hd/bta_hd_act.c"
                   "host/bluedroid/bta/hd/bta_hd_main.c"
                   "host/bluedroid/bta/hh/bta_hh_act.c"
                   "host/bluedroid/bta/hh/bta_hh_api.c"
                   "host/bluedroid/bta/hh/bta_hh_cfg.c"
                   "host/bluedroid/bta/hh/bta_hh_le.c"
                   "host/bluedroid/bta/hh/bta_hh_main.c"
                   "host/bluedroid/bta/hh/bta_hh_utils.c"
                   "host/bluedroid/bta/jv/bta_jv_act.c"
                   "host/bluedroid/bta/jv/bta_jv_api.c"
                   "host/bluedroid/bta/jv/bta_jv_cfg.c"
                   "host/bluedroid/bta/jv/bta_jv_main.c"
                   "host/bluedroid/bta/hf_ag/bta_ag_act.c"
                   "host/bluedroid/bta/hf_ag/bta_ag_api.c"
                   "host/bluedroid/bta/hf_ag/bta_ag_at.c"
                   "host/bluedroid/bta/hf_ag/bta_ag_cfg.c"
                   "host/bluedroid/bta/hf_ag/bta_ag_cmd.c"
                   "host/bluedroid/bta/hf_ag/bta_ag_main.c"
                   "host/bluedroid/bta/hf_ag/bta_ag_rfc.c"
                   "host/bluedroid/bta/hf_ag/bta_ag_sco.c"
                   "host/bluedroid/bta/hf_ag/bta_ag_sdp.c"
                   "host/bluedroid/bta/hf_client/bta_hf_client_act.c"
                   "host/bluedroid/bta/hf_client/bta_hf_client_api.c"
                   "host/bluedroid/bta/hf_client/bta_hf_client_at.c"
                   "host/bluedroid/bta/hf_client/bta_hf_client_cmd.c"
                   "host/bluedroid/bta/hf_client/bta_hf_client_main.c"
                   "host/bluedroid/bta/hf_client/bta_hf_client_rfc.c"
                   "host/bluedroid/bta/hf_client/bta_hf_client_sco.c"
                   "host/bluedroid/bta/hf_client/bta_hf_client_sdp.c"
                   "host/bluedroid/bta/sdp/bta_sdp.c"
                   "host/bluedroid/bta/sdp/bta_sdp_act.c"
                   "host/bluedroid/bta/sdp/bta_sdp_api.c"
                   "host/bluedroid/bta/sdp/bta_sdp_cfg.c"
                   "host/bluedroid/bta/sys/bta_sys_conn.c"
                   "host/bluedroid/bta/sys/bta_sys_main.c"
                   "host/bluedroid/bta/sys/utl.c"
                   "host/bluedroid/btc/core/btc_ble_storage.c"
                   "host/bluedroid/btc/core/btc_config.c"
                   "host/bluedroid/btc/core/btc_dev.c"
                   "host/bluedroid/btc/core/btc_dm.c"
                   "host/bluedroid/btc/core/btc_main.c"
                   "host/bluedroid/btc/core/btc_profile_queue.c"
                   "host/bluedroid/btc/core/btc_sec.c"
                   "host/bluedroid/btc/core/btc_sm.c"
                   "host/bluedroid/btc/core/btc_storage.c"
                   "host/bluedroid/btc/core/btc_util.c"
                   "host/bluedroid/btc/profile/std/a2dp/bta_av_co.c"
                   "host/bluedroid/btc/profile/std/a2dp/btc_a2dp.c"
                   "host/bluedroid/btc/profile/std/a2dp/btc_a2dp_control.c"
                   "host/bluedroid/btc/profile/std/a2dp/btc_a2dp_sink.c"
                   "host/bluedroid/btc/profile/std/a2dp/btc_a2dp_source.c"
                   "host/bluedroid/btc/profile/std/a2dp/btc_av.c"
                   "host/bluedroid/btc/profile/std/avrc/btc_avrc.c"
                   "host/bluedroid/btc/profile/std/avrc/bta_avrc_co.c"
                   "host/bluedroid/btc/profile/std/hf_ag/bta_ag_co.c"
                   "host/bluedroid/btc/profile/std/hf_ag/btc_hf_ag.c"
                   "host/bluedroid/btc/profile/std/hf_client/btc_hf_client.c"
                   "host/bluedroid/btc/profile/std/hf_client/bta_hf_client_co.c"
                   "host/bluedroid/btc/profile/std/hid/btc_hd.c"
                   "host/bluedroid/btc/profile/std/hid/btc_hh.c"
                   "host/bluedroid/btc/profile/std/hid/bta_hh_co.c"
                   "host/bluedroid/btc/profile/std/gap/btc_gap_ble.c"
                   "host/bluedroid/btc/profile/std/gap/btc_gap_bt.c"
                   "host/bluedroid/btc/profile/std/gap/bta_gap_bt_co.c"
                   "host/bluedroid/btc/profile/std/gatt/btc_gatt_common.c"
                   "host/bluedroid/btc/profile/std/gatt/btc_gatt_util.c"
                   "host/bluedroid/btc/profile/std/gatt/btc_gattc.c"
                   "host/bluedroid/btc/profile/std/gatt/btc_gatts.c"
                   "host/bluedroid/btc/profile/std/spp/btc_spp.c"
                   "host/bluedroid/device/bdaddr.c"
                   "host/bluedroid/device/controller.c"
                   "host/bluedroid/device/interop.c"
                   "host/bluedroid/external/sbc/decoder/srce/alloc.c"
                   "host/bluedroid/external/sbc/decoder/srce/bitalloc-sbc.c"
                   "host/bluedroid/external/sbc/decoder/srce/bitalloc.c"
                   "host/bluedroid/external/sbc/decoder/srce/bitstream-decode.c"
                   "host/bluedroid/external/sbc/decoder/srce/decoder-oina.c"
                   "host/bluedroid/external/sbc/decoder/srce/decoder-private.c"
                   "host/bluedroid/external/sbc/decoder/srce/decoder-sbc.c"
                   "host/bluedroid/external/sbc/decoder/srce/dequant.c"
                   "host/bluedroid/external/sbc/decoder/srce/framing-sbc.c"
                   "host/bluedroid/external/sbc/decoder/srce/framing.c"
                   "host/bluedroid/external/sbc/decoder/srce/oi_codec_version.c"
                   "host/bluedroid/external/sbc/decoder/srce/synthesis-8-generated.c"
                   "host/bluedroid/external/sbc/decoder/srce/synthesis-dct8.c"
                   "host/bluedroid/external/sbc/decoder/srce/synthesis-sbc.c"
                   "host/bluedroid/external/sbc/encoder/srce/sbc_analysis.c"
                   "host/bluedroid/external/sbc/encoder/srce/sbc_dct.c"
                   "host/bluedroid/external/sbc/encoder/srce/sbc_dct_coeffs.c"
                   "host/bluedroid/external/sbc/encoder/srce/sbc_enc_bit_alloc_mono.c"
                   "host/bluedroid/external/sbc/encoder/srce/sbc_enc_bit_alloc_ste.c"
                   "host/bluedroid/external/sbc/encoder/srce/sbc_enc_coeffs.c"
                   "host/bluedroid/external/sbc/encoder/srce/sbc_encoder.c"
                   "host/bluedroid/external/sbc/encoder/srce/sbc_packing.c"
                   "host/bluedroid/external/sbc/plc/sbc_plc.c"
                   "host/bluedroid/hci/hci_audio.c"
                   "host/bluedroid/hci/hci_hal_h4.c"
                   "host/bluedroid/hci/hci_layer.c"
                   "host/bluedroid/hci/hci_packet_factory.c"
                   "host/bluedroid/hci/hci_packet_parser.c"
                   "host/bluedroid/hci/packet_fragmenter.c"
                   "host/bluedroid/main/bte_init.c"
                   "host/bluedroid/main/bte_main.c"
                   "host/bluedroid/stack/a2dp/a2d_api.c"
                   "host/bluedroid/stack/a2dp/a2d_sbc.c"
                   "host/bluedroid/stack/avct/avct_api.c"
                   "host/bluedroid/stack/avct/avct_ccb.c"
                   "host/bluedroid/stack/avct/avct_l2c.c"
                   "host/bluedroid/stack/avct/avct_lcb.c"
                   "host/bluedroid/stack/avct/avct_lcb_act.c"
                   "host/bluedroid/stack/avdt/avdt_ad.c"
                   "host/bluedroid/stack/avdt/avdt_api.c"
                   "host/bluedroid/stack/avdt/avdt_ccb.c"
                   "host/bluedroid/stack/avdt/avdt_ccb_act.c"
                   "host/bluedroid/stack/avdt/avdt_l2c.c"
                   "host/bluedroid/stack/avdt/avdt_msg.c"
                   "host/bluedroid/stack/avdt/avdt_scb.c"
                   "host/bluedroid/stack/avdt/avdt_scb_act.c"
                   "host/bluedroid/stack/avrc/avrc_api.c"
                   "host/bluedroid/stack/avrc/avrc_bld_ct.c"
                   "host/bluedroid/stack/avrc/avrc_bld_tg.c"
                   "host/bluedroid/stack/avrc/avrc_opt.c"
                   "host/bluedroid/stack/avrc/avrc_pars_ct.c"
                   "host/bluedroid/stack/avrc/avrc_pars_tg.c"
                   "host/bluedroid/stack/avrc/avrc_sdp.c"
                   "host/bluedroid/stack/avrc/avrc_utils.c"
                   "host/bluedroid/stack/hid/hidd_api.c"
                   "host/bluedroid/stack/hid/hidd_conn.c"
                   "host/bluedroid/stack/hid/hidh_api.c"
                   "host/bluedroid/stack/hid/hidh_conn.c"
                   "host/bluedroid/stack/btm/btm_acl.c"
                   "host/bluedroid/stack/btm/btm_ble.c"
                   "host/bluedroid/stack/btm/btm_ble_addr.c"
                   "host/bluedroid/stack/btm/btm_ble_adv_filter.c"
                   "host/bluedroid/stack/btm/btm_ble_batchscan.c"
                   "host/bluedroid/stack/btm/btm_ble_bgconn.c"
                   "host/bluedroid/stack/btm/btm_ble_cont_energy.c"
                   "host/bluedroid/stack/btm/btm_ble_gap.c"
                   "host/bluedroid/stack/btm/btm_ble_5_gap.c"
                   "host/bluedroid/stack/btm/btm_ble_multi_adv.c"
                   "host/bluedroid/stack/btm/btm_ble_privacy.c"
                   "host/bluedroid/stack/btm/btm_dev.c"
                   "host/bluedroid/stack/btm/btm_devctl.c"
                   "host/bluedroid/stack/btm/btm_inq.c"
                   "host/bluedroid/stack/btm/btm_main.c"
                   "host/bluedroid/stack/btm/btm_pm.c"
                   "host/bluedroid/stack/btm/btm_sco.c"
                   "host/bluedroid/stack/btm/btm_sec.c"
                   "host/bluedroid/stack/btu/btu_hcif.c"
                   "host/bluedroid/stack/btu/btu_init.c"
                   "host/bluedroid/stack/btu/btu_task.c"
                   "host/bluedroid/stack/gap/gap_api.c"
                   "host/bluedroid/stack/gap/gap_ble.c"
                   "host/bluedroid/stack/gap/gap_conn.c"
                   "host/bluedroid/stack/gap/gap_utils.c"
                   "host/bluedroid/stack/gatt/att_protocol.c"
                   "host/bluedroid/stack/gatt/gatt_api.c"
                   "host/bluedroid/stack/gatt/gatt_attr.c"
                   "host/bluedroid/stack/gatt/gatt_auth.c"
                   "host/bluedroid/stack/gatt/gatt_cl.c"
                   "host/bluedroid/stack/gatt/gatt_db.c"
                   "host/bluedroid/stack/gatt/gatt_main.c"
                   "host/bluedroid/stack/gatt/gatt_sr.c"
                   "host/bluedroid/stack/gatt/gatt_utils.c"
                   "host/bluedroid/stack/hcic/hciblecmds.c"
                   "host/bluedroid/stack/hcic/hcicmds.c"
                   "host/bluedroid/stack/l2cap/l2c_api.c"
                   "host/bluedroid/stack/l2cap/l2c_ble.c"
                   "host/bluedroid/stack/l2cap/l2c_csm.c"
                   "host/bluedroid/stack/l2cap/l2c_fcr.c"
                   "host/bluedroid/stack/l2cap/l2c_link.c"
                   "host/bluedroid/stack/l2cap/l2c_main.c"
                   "host/bluedroid/stack/l2cap/l2c_ucd.c"
                   "host/bluedroid/stack/l2cap/l2c_utils.c"
                   "host/bluedroid/stack/l2cap/l2cap_client.c"
                   "host/bluedroid/stack/rfcomm/port_api.c"
                   "host/bluedroid/stack/rfcomm/port_rfc.c"
                   "host/bluedroid/stack/rfcomm/port_utils.c"
                   "host/bluedroid/stack/rfcomm/rfc_l2cap_if.c"
                   "host/bluedroid/stack/rfcomm/rfc_mx_fsm.c"
                   "host/bluedroid/stack/rfcomm/rfc_port_fsm.c"
                   "host/bluedroid/stack/rfcomm/rfc_port_if.c"
                   "host/bluedroid/stack/rfcomm/rfc_ts_frames.c"
                   "host/bluedroid/stack/rfcomm/rfc_utils.c"
                   "host/bluedroid/stack/sdp/sdp_api.c"
                   "host/bluedroid/stack/sdp/sdp_db.c"
                   "host/bluedroid/stack/sdp/sdp_discovery.c"
                   "host/bluedroid/stack/sdp/sdp_main.c"
                   "host/bluedroid/stack/sdp/sdp_server.c"
                   "host/bluedroid/stack/sdp/sdp_utils.c"
                   "host/bluedroid/stack/smp/aes.c"
                   "host/bluedroid/stack/smp/p_256_curvepara.c"
                   "host/bluedroid/stack/smp/p_256_ecc_pp.c"
                   "host/bluedroid/stack/smp/p_256_multprecision.c"
                   "host/bluedroid/stack/smp/smp_act.c"
                   "host/bluedroid/stack/smp/smp_api.c"
                   "host/bluedroid/stack/smp/smp_br_main.c"
                   "host/bluedroid/stack/smp/smp_cmac.c"
                   "host/bluedroid/stack/smp/smp_keys.c"
                   "host/bluedroid/stack/smp/smp_l2c.c"
                   "host/bluedroid/stack/smp/smp_main.c"
                   "host/bluedroid/stack/smp/smp_utils.c")

            list(APPEND srcs "common/btc/profile/esp/blufi/bluedroid_host/esp_blufi.c")
        if(CONFIG_BLE_MESH)
            list(APPEND srcs "esp_ble_mesh/mesh_core/bluedroid_host/mesh_bearer_adapt.c")
        endif()

    endif()

    if(CONFIG_BLE_MESH)
        list(APPEND include_dirs
                    "esp_ble_mesh/mesh_common/include"
                    "esp_ble_mesh/mesh_common/tinycrypt/include"
                    "esp_ble_mesh/mesh_core"
                    "esp_ble_mesh/mesh_core/include"
                    "esp_ble_mesh/mesh_core/storage"
                    "esp_ble_mesh/btc/include"
                    "esp_ble_mesh/mesh_models/common/include"
                    "esp_ble_mesh/mesh_models/client/include"
                    "esp_ble_mesh/mesh_models/server/include"
                    "esp_ble_mesh/api/core/include"
                    "esp_ble_mesh/api/models/include"
                    "esp_ble_mesh/api")

        list(APPEND srcs "esp_ble_mesh/api/core/esp_ble_mesh_ble_api.c"
                    "esp_ble_mesh/api/core/esp_ble_mesh_common_api.c"
                    "esp_ble_mesh/api/core/esp_ble_mesh_local_data_operation_api.c"
                    "esp_ble_mesh/api/core/esp_ble_mesh_low_power_api.c"
                    "esp_ble_mesh/api/core/esp_ble_mesh_networking_api.c"
                    "esp_ble_mesh/api/core/esp_ble_mesh_provisioning_api.c"
                    "esp_ble_mesh/api/core/esp_ble_mesh_proxy_api.c"
                    "esp_ble_mesh/api/models/esp_ble_mesh_config_model_api.c"
                    "esp_ble_mesh/api/models/esp_ble_mesh_generic_model_api.c"
                    "esp_ble_mesh/api/models/esp_ble_mesh_health_model_api.c"
                    "esp_ble_mesh/api/models/esp_ble_mesh_lighting_model_api.c"
                    "esp_ble_mesh/api/models/esp_ble_mesh_sensor_model_api.c"
                    "esp_ble_mesh/api/models/esp_ble_mesh_time_scene_model_api.c"
                    "esp_ble_mesh/btc/btc_ble_mesh_ble.c"
                    "esp_ble_mesh/btc/btc_ble_mesh_config_model.c"
                    "esp_ble_mesh/btc/btc_ble_mesh_generic_model.c"
                    "esp_ble_mesh/btc/btc_ble_mesh_health_model.c"
                    "esp_ble_mesh/btc/btc_ble_mesh_lighting_model.c"
                    "esp_ble_mesh/btc/btc_ble_mesh_prov.c"
                    "esp_ble_mesh/btc/btc_ble_mesh_sensor_model.c"
                    "esp_ble_mesh/btc/btc_ble_mesh_time_scene_model.c"
                    "esp_ble_mesh/mesh_common/tinycrypt/src/aes_decrypt.c"
                    "esp_ble_mesh/mesh_common/tinycrypt/src/aes_encrypt.c"
                    "esp_ble_mesh/mesh_common/tinycrypt/src/cbc_mode.c"
                    "esp_ble_mesh/mesh_common/tinycrypt/src/ccm_mode.c"
                    "esp_ble_mesh/mesh_common/tinycrypt/src/cmac_mode.c"
                    "esp_ble_mesh/mesh_common/tinycrypt/src/ctr_mode.c"
                    "esp_ble_mesh/mesh_common/tinycrypt/src/ctr_prng.c"
                    "esp_ble_mesh/mesh_common/tinycrypt/src/ecc_dh.c"
                    "esp_ble_mesh/mesh_common/tinycrypt/src/ecc_dsa.c"
                    "esp_ble_mesh/mesh_common/tinycrypt/src/ecc_platform_specific.c"
                    "esp_ble_mesh/mesh_common/tinycrypt/src/ecc.c"
                    "esp_ble_mesh/mesh_common/tinycrypt/src/hmac_prng.c"
                    "esp_ble_mesh/mesh_common/tinycrypt/src/hmac.c"
                    "esp_ble_mesh/mesh_common/tinycrypt/src/sha256.c"
                    "esp_ble_mesh/mesh_common/tinycrypt/src/utils.c"
                    "esp_ble_mesh/mesh_common/mesh_atomic.c"
                    "esp_ble_mesh/mesh_common/mesh_buf.c"
                    "esp_ble_mesh/mesh_common/mesh_common.c"
                    "esp_ble_mesh/mesh_common/mesh_kernel.c"
                    "esp_ble_mesh/mesh_common/mesh_mutex.c"
                    "esp_ble_mesh/mesh_common/mesh_timer.c"
                    "esp_ble_mesh/mesh_common/mesh_util.c"
                    "esp_ble_mesh/mesh_core/storage/settings_nvs.c"
                    "esp_ble_mesh/mesh_core/access.c"
                    "esp_ble_mesh/mesh_core/adv.c"
                    "esp_ble_mesh/mesh_core/beacon.c"
                    "esp_ble_mesh/mesh_core/cfg_cli.c"
                    "esp_ble_mesh/mesh_core/cfg_srv.c"
                    "esp_ble_mesh/mesh_core/crypto.c"
                    "esp_ble_mesh/mesh_core/fast_prov.c"
                    "esp_ble_mesh/mesh_core/friend.c"
                    "esp_ble_mesh/mesh_core/health_cli.c"
                    "esp_ble_mesh/mesh_core/health_srv.c"
                    "esp_ble_mesh/mesh_core/local_operation.c"
                    "esp_ble_mesh/mesh_core/lpn.c"
                    "esp_ble_mesh/mesh_core/main.c"
                    "esp_ble_mesh/mesh_core/net.c"
                    "esp_ble_mesh/mesh_core/prov.c"
                    "esp_ble_mesh/mesh_core/provisioner_main.c"
                    "esp_ble_mesh/mesh_core/provisioner_prov.c"
                    "esp_ble_mesh/mesh_core/proxy_client.c"
                    "esp_ble_mesh/mesh_core/proxy_server.c"
                    "esp_ble_mesh/mesh_core/settings_uid.c"
                    "esp_ble_mesh/mesh_core/settings.c"
                    "esp_ble_mesh/mesh_core/scan.c"
                    "esp_ble_mesh/mesh_core/test.c"
                    "esp_ble_mesh/mesh_core/transport.c"
                    "esp_ble_mesh/mesh_models/common/device_property.c"
                    "esp_ble_mesh/mesh_models/client/client_common.c"
                    "esp_ble_mesh/mesh_models/client/generic_client.c"
                    "esp_ble_mesh/mesh_models/client/lighting_client.c"
                    "esp_ble_mesh/mesh_models/client/sensor_client.c"
                    "esp_ble_mesh/mesh_models/client/time_scene_client.c"
                    "esp_ble_mesh/mesh_models/server/generic_server.c"
                    "esp_ble_mesh/mesh_models/server/lighting_server.c"
                    "esp_ble_mesh/mesh_models/server/sensor_server.c"
                    "esp_ble_mesh/mesh_models/server/server_common.c"
                    "esp_ble_mesh/mesh_models/server/state_binding.c"
                    "esp_ble_mesh/mesh_models/server/state_transition.c"
                    "esp_ble_mesh/mesh_models/server/time_scene_server.c")
    endif()

    if(CONFIG_BT_NIMBLE_ENABLED)

        list(APPEND include_dirs
                    host/nimble/nimble/porting/nimble/include
                    host/nimble/port/include
                    host/nimble/nimble/nimble/include
                    host/nimble/nimble/nimble/host/include
                    host/nimble/nimble/nimble/host/services/ans/include
                    host/nimble/nimble/nimble/host/services/bas/include
                    host/nimble/nimble/nimble/host/services/dis/include
                    host/nimble/nimble/nimble/host/services/gap/include
                    host/nimble/nimble/nimble/host/services/gatt/include
                    host/nimble/nimble/nimble/host/services/ias/include
                    host/nimble/nimble/nimble/host/services/ipss/include
                    host/nimble/nimble/nimble/host/services/lls/include
                    host/nimble/nimble/nimble/host/services/tps/include
                    host/nimble/nimble/nimble/host/util/include
                    host/nimble/nimble/nimble/host/store/ram/include
                    host/nimble/nimble/nimble/host/store/config/include
                    host/nimble/nimble/porting/npl/freertos/include
                    host/nimble/esp-hci/include)

    if(NOT CONFIG_BT_NIMBLE_CRYPTO_STACK_MBEDTLS)

        list(APPEND include_dirs
                    host/nimble/nimble/ext/tinycrypt/include)

        list(APPEND srcs "host/nimble/nimble/ext/tinycrypt/src/utils.c"
                    "host/nimble/nimble/ext/tinycrypt/src/sha256.c"
                    "host/nimble/nimble/ext/tinycrypt/src/ecc.c"
                    "host/nimble/nimble/ext/tinycrypt/src/ctr_prng.c"
                    "host/nimble/nimble/ext/tinycrypt/src/ctr_mode.c"
                    "host/nimble/nimble/ext/tinycrypt/src/aes_decrypt.c"
                    "host/nimble/nimble/ext/tinycrypt/src/aes_encrypt.c"
                    "host/nimble/nimble/ext/tinycrypt/src/ccm_mode.c"
                    "host/nimble/nimble/ext/tinycrypt/src/ecc_dsa.c"
                    "host/nimble/nimble/ext/tinycrypt/src/cmac_mode.c"
                    "host/nimble/nimble/ext/tinycrypt/src/ecc_dh.c"
                    "host/nimble/nimble/ext/tinycrypt/src/hmac_prng.c"
                    "host/nimble/nimble/ext/tinycrypt/src/ecc_platform_specific.c"
                    "host/nimble/nimble/ext/tinycrypt/src/hmac.c"
                    "host/nimble/nimble/ext/tinycrypt/src/cbc_mode.c")
    endif()

        list(APPEND srcs "host/nimble/nimble/nimble/host/util/src/addr.c"
                    "host/nimble/nimble/nimble/host/services/gatt/src/ble_svc_gatt.c"
                    "host/nimble/nimble/nimble/host/services/tps/src/ble_svc_tps.c"
                    "host/nimble/nimble/nimble/host/services/ias/src/ble_svc_ias.c"
                    "host/nimble/nimble/nimble/host/services/ipss/src/ble_svc_ipss.c"
                    "host/nimble/nimble/nimble/host/services/ans/src/ble_svc_ans.c"
                    "host/nimble/nimble/nimble/host/services/gap/src/ble_svc_gap.c"
                    "host/nimble/nimble/nimble/host/services/bas/src/ble_svc_bas.c"
                    "host/nimble/nimble/nimble/host/services/dis/src/ble_svc_dis.c"
                    "host/nimble/nimble/nimble/host/services/lls/src/ble_svc_lls.c"
                    "host/nimble/nimble/nimble/host/src/ble_hs_conn.c"
                    "host/nimble/nimble/nimble/host/src/ble_store_util.c"
                    "host/nimble/nimble/nimble/host/src/ble_sm.c"
                    "host/nimble/nimble/nimble/host/src/ble_hs_shutdown.c"
                    "host/nimble/nimble/nimble/host/src/ble_l2cap_sig_cmd.c"
                    "host/nimble/nimble/nimble/host/src/ble_hs_hci_cmd.c"
                    "host/nimble/nimble/nimble/host/src/ble_hs_id.c"
                    "host/nimble/nimble/nimble/host/src/ble_att_svr.c"
                    "host/nimble/nimble/nimble/host/src/ble_gatts_lcl.c"
                    "host/nimble/nimble/nimble/host/src/ble_ibeacon.c"
                    "host/nimble/nimble/nimble/host/src/ble_hs_atomic.c"
                    "host/nimble/nimble/nimble/host/src/ble_sm_alg.c"
                    "host/nimble/nimble/nimble/host/src/ble_hs_stop.c"
                    "host/nimble/nimble/nimble/host/src/ble_hs.c"
                    "host/nimble/nimble/nimble/host/src/ble_hs_hci_evt.c"
                    "host/nimble/nimble/nimble/host/src/ble_hs_mqueue.c"
                    "host/nimble/nimble/nimble/host/src/ble_hs_periodic_sync.c"
                    "host/nimble/nimble/nimble/host/src/ble_att.c"
                    "host/nimble/nimble/nimble/host/src/ble_gattc.c"
                    "host/nimble/nimble/nimble/host/src/ble_store.c"
                    "host/nimble/nimble/nimble/host/src/ble_sm_lgcy.c"
                    "host/nimble/nimble/nimble/host/src/ble_hs_cfg.c"
                    "host/nimble/nimble/nimble/host/src/ble_monitor.c"
                    "host/nimble/nimble/nimble/host/src/ble_att_clt.c"
                    "host/nimble/nimble/nimble/host/src/ble_l2cap_coc.c"
                    "host/nimble/nimble/nimble/host/src/ble_hs_mbuf.c"
                    "host/nimble/nimble/nimble/host/src/ble_att_cmd.c"
                    "host/nimble/nimble/nimble/host/src/ble_hs_log.c"
                    "host/nimble/nimble/nimble/host/src/ble_eddystone.c"
                    "host/nimble/nimble/nimble/host/src/ble_hs_startup.c"
                    "host/nimble/nimble/nimble/host/src/ble_l2cap_sig.c"
                    "host/nimble/nimble/nimble/host/src/ble_gap.c"
                    "host/nimble/nimble/nimble/host/src/ble_sm_cmd.c"
                    "host/nimble/nimble/nimble/host/src/ble_uuid.c"
                    "host/nimble/nimble/nimble/host/src/ble_hs_pvcy.c"
                    "host/nimble/nimble/nimble/host/src/ble_hs_flow.c"
                    "host/nimble/nimble/nimble/host/src/ble_l2cap.c"
                    "host/nimble/nimble/nimble/host/src/ble_sm_sc.c"
                    "host/nimble/nimble/nimble/host/src/ble_hs_misc.c"
                    "host/nimble/nimble/nimble/host/src/ble_gatts.c"
                    "host/nimble/nimble/nimble/host/src/ble_hs_adv.c"
                    "host/nimble/nimble/nimble/host/src/ble_hs_hci.c"
                    "host/nimble/nimble/nimble/host/src/ble_hs_hci_util.c"
                    "host/nimble/nimble/nimble/host/src/ble_hs_resolv.c"
                    "host/nimble/nimble/nimble/host/store/ram/src/ble_store_ram.c"
                    "host/nimble/nimble/nimble/host/store/config/src/ble_store_config.c"
                    "host/nimble/nimble/nimble/host/store/config/src/ble_store_nvs.c"
                    "host/nimble/nimble/porting/npl/freertos/src/nimble_port_freertos.c"
                    "host/nimble/nimble/porting/npl/freertos/src/npl_os_freertos.c"
                    "host/nimble/nimble/porting/nimble/src/endian.c"
                    "host/nimble/nimble/porting/nimble/src/os_cputime_pwr2.c"
                    "host/nimble/nimble/porting/nimble/src/hal_timer.c"
                    "host/nimble/nimble/porting/nimble/src/os_mempool.c"
                    "host/nimble/nimble/porting/nimble/src/os_msys_init.c"
                    "host/nimble/nimble/porting/nimble/src/nimble_port.c"
                    "host/nimble/nimble/porting/nimble/src/mem.c"
                    "host/nimble/nimble/porting/nimble/src/os_mbuf.c"
                    "host/nimble/nimble/porting/nimble/src/os_cputime.c"
                    "host/nimble/esp-hci/src/esp_nimble_hci.c"
                    "host/nimble/port/src/esp_nimble_mem.c")

            list(APPEND srcs
                    "common/btc/profile/esp/blufi/nimble_host/esp_blufi.c")
        if(CONFIG_BLE_MESH)
            list(APPEND srcs "esp_ble_mesh/mesh_core/nimble_host/mesh_bearer_adapt.c")
        endif()

        if(CONFIG_BT_NIMBLE_MESH)

            list(APPEND include_dirs
                host/nimble/nimble/nimble/host/mesh/include)

            list(APPEND srcs "host/nimble/nimble/nimble/host/mesh/src/shell.c"
                        "host/nimble/nimble/nimble/host/mesh/src/friend.c"
                        "host/nimble/nimble/nimble/host/mesh/src/crypto.c"
                        "host/nimble/nimble/nimble/host/mesh/src/settings.c"
                        "host/nimble/nimble/nimble/host/mesh/src/adv.c"
                        "host/nimble/nimble/nimble/host/mesh/src/model_srv.c"
                        "host/nimble/nimble/nimble/host/mesh/src/beacon.c"
                        "host/nimble/nimble/nimble/host/mesh/src/glue.c"
                        "host/nimble/nimble/nimble/host/mesh/src/model_cli.c"
                        "host/nimble/nimble/nimble/host/mesh/src/transport.c"
                        "host/nimble/nimble/nimble/host/mesh/src/prov.c"
                        "host/nimble/nimble/nimble/host/mesh/src/mesh.c"
                        "host/nimble/nimble/nimble/host/mesh/src/nodes.c"
                        "host/nimble/nimble/nimble/host/mesh/src/access.c"
                        "host/nimble/nimble/nimble/host/mesh/src/cfg_srv.c"
                        "host/nimble/nimble/nimble/host/mesh/src/cfg_cli.c"
                        "host/nimble/nimble/nimble/host/mesh/src/light_model.c"
                        "host/nimble/nimble/nimble/host/mesh/src/health_cli.c"
                        "host/nimble/nimble/nimble/host/mesh/src/lpn.c"
                        "host/nimble/nimble/nimble/host/mesh/src/proxy.c"
                        "host/nimble/nimble/nimble/host/mesh/src/health_srv.c"
                        "host/nimble/nimble/nimble/host/mesh/src/testing.c"
                        "host/nimble/nimble/nimble/host/mesh/src/net.c")

        endif()
    endif()

endif()

# requirements can't depend on config
idf_component_register(SRCS "${srcs}"
                INCLUDE_DIRS "${include_dirs}"
                PRIV_INCLUDE_DIRS "${priv_include_dirs}"
                REQUIRES nvs_flash soc esp_timer esp_pm esp_phy
                PRIV_REQUIRES esp_ipc
                LDFRAGMENTS "linker.lf")

if(CONFIG_BT_ENABLED)
    target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-implicit-fallthrough -Wno-unused-const-variable)
    if(CONFIG_IDF_TARGET_ESP32)
        target_link_libraries(${COMPONENT_LIB} INTERFACE "-L${CMAKE_CURRENT_LIST_DIR}/controller/lib_esp32/esp32")
        target_link_libraries(${COMPONENT_LIB} PUBLIC btdm_app)

        target_link_libraries(${COMPONENT_LIB} INTERFACE "-u ld_include_hli_vectors_bt")
    elseif(CONFIG_IDF_TARGET_ESP32C3)
        target_link_libraries(${COMPONENT_LIB} INTERFACE
                "-L${CMAKE_CURRENT_LIST_DIR}/controller/lib_esp32c3_family/esp32c3")
        target_link_libraries(${COMPONENT_LIB} PUBLIC btdm_app)
    elseif(CONFIG_IDF_TARGET_ESP32S3)
        target_link_libraries(${COMPONENT_LIB} INTERFACE
                "-L${CMAKE_CURRENT_LIST_DIR}/controller/lib_esp32c3_family/esp32s3")
        target_link_libraries(${COMPONENT_LIB} PUBLIC btdm_app)
    endif()
endif()

if(CONFIG_BT_NIMBLE_MESH)
    set_source_files_properties("host/nimble/nimble/nimble/host/mesh/src/net.c"
        PROPERTIES COMPILE_FLAGS -Wno-type-limits)
endif()

if(CONFIG_COMPILER_OPTIMIZATION_ASSERTIONS_DISABLE AND CONFIG_BT_NIMBLE_ENABLED)
    # some variables in NimBLE are only used by asserts
    target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-but-set-variable -Wno-unused-variable)
endif()

if(NOT CMAKE_BUILD_EARLY_EXPANSION)
    set_source_files_properties("${CMAKE_CURRENT_LIST_DIR}/host/bluedroid/bta/hf_ag/bta_ag_cmd.c"
                                "${CMAKE_CURRENT_LIST_DIR}/host/bluedroid/btc/profile/std/gap/btc_gap_ble.c"
                                 PROPERTIES COMPILE_FLAGS "-fjump-tables -ftree-switch-conversion")
endif()
