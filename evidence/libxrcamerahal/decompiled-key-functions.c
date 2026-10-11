// C:\Users\remus\quest3-ir-passthrough\ota-inspect\libs\libxrcamerahal.so
// matched 6 functions for targets: ['GetSupportedCameraIds', 'onIsBayerCamera', 'onGetCameraType', 'overrideMonoCameraConfigurationFromSystemProperties', 'overrideBayerFeatureSetFromSystemProperties', 'onGetEnabledStreamTypeList', 'GetSupportedCameraIdsEv']

====================================================================================================
// overrideMonoCameraConfigurationFromSystemProperties  @ 00133960

/* CameraHal::XrCameraConfiguration::overrideMonoCameraConfigurationFromSystemProperties() */

void __thiscall
CameraHal::XrCameraConfiguration::overrideMonoCameraConfigurationFromSystemProperties
          (XrCameraConfiguration *this)

{
  __hash_table<PipelineControlBase::Feature,std::hash<PipelineControlBase::Feature>,std::equal_to<PipelineControlBase::Feature>,std::allocator<PipelineControlBase::Feature>>
  *this_00;
  XrCameraConfiguration *pXVar1;
  uint uVar2;
  XrCameraConfiguration *pXVar3;
  uint uVar4;
  ulong uVar5;
  long lVar6;
  void *pvVar7;
  undefined8 *puVar8;
  bool bVar9;
  ushort uVar10;
  ushort uVar11;
  ushort uVar12;
  ushort uVar13;
  undefined4 uVar14;
  long lVar15;
  size_t sVar16;
  ulong uVar17;
  ulong uVar18;
  ulong uVar19;
  long *plVar20;
  ulong uVar21;
  void *pvVar22;
  long lVar23;
  char *__s;
  undefined2 uVar24;
  undefined8 uVar25;
  piecewise_construct_t apStack_a8 [8];
  piecewise_construct_t *local_a0;
  undefined1 auStack_98 [8];
  void *local_90;
  undefined8 uStack_88;
  undefined8 *local_80;
  undefined8 uStack_78;
  undefined8 local_70;
  long local_68;
  
  lVar6 = tpidr_el0;
  local_68 = *(long *)(lVar6 + 0x28);
  if (*(char *)(*(long *)this + 0x40) == '\0') {
    uVar10 = oculus::sysprop::XrtHalQcProperties::enable_fastpath();
    if (uVar10 < 0x101) {
      uVar10 = 0;
    }
    uVar11 = oculus::sysprop::XrtHalQcProperties::mono_use_gamma();
    if (uVar11 < 0x101) {
      uVar11 = 0;
    }
    uVar12 = oculus::sysprop::XrtHalQcProperties::enable_secure();
    if (uVar12 < 0x101) {
      uVar12 = 0;
    }
    uVar13 = oculus::sysprop::XrtHalQcProperties::enable_ultralite();
    if (uVar13 < 0x101) {
      uVar13 = 0;
    }
    if (((uVar10 & 0xff) != 0) && ((uVar11 & 0xff) != 0)) {
                    /* WARNING: Subroutine does not return */
      __android_log_assert
                ("isFastpath && enableMonoGamma","XrCameraHal",
                 "You cannot have fastpath and ife-lite gamma compression! Pick either one!");
    }
    if (((uVar10 & 0xff) != 0) && ((uVar12 & 0xff) != 0)) {
                    /* WARNING: Subroutine does not return */
      __android_log_assert
                ("isFastpath && enableSecureCamera","XrCameraHal",
                 "You cannot have fastpath and ife-lite secure camera! Pick either one!");
    }
    if (((uVar10 & 0xff) != 0) && ((uVar13 & 0xff) != 0)) {
                    /* WARNING: Subroutine does not return */
      __android_log_assert
                ("isFastpath && enableUltraLite","XrCameraHal",
                 "You cannot have fastpath and ultraLite enabled! Pick either one!");
    }
    this_00 = (__hash_table<PipelineControlBase::Feature,std::hash<PipelineControlBase::Feature>,std::equal_to<PipelineControlBase::Feature>,std::allocator<PipelineControlBase::Feature>>
               *)(this + 0x188);
    if ((uVar10 & 0xff) == 0) {
      uVar17 = *(ulong *)(this + 400);
      if (uVar17 != 0) {
        uVar25 = CONCAT17(POPCOUNT((char)(uVar17 >> 0x38)),
                          CONCAT16(POPCOUNT((char)(uVar17 >> 0x30)),
                                   CONCAT15(POPCOUNT((char)(uVar17 >> 0x28)),
                                            CONCAT14(POPCOUNT((char)(uVar17 >> 0x20)),
                                                     CONCAT13(POPCOUNT((char)(uVar17 >> 0x18)),
                                                              CONCAT12(POPCOUNT((char)(uVar17 >>
                                                                                      0x10)),
                                                                       CONCAT11(POPCOUNT((char)(
                                                  uVar17 >> 8)),POPCOUNT((char)uVar17))))))));
        uVar24 = NEON_uaddlv(uVar25,1);
        uVar18 = CONCAT62((int6)((ulong)uVar25 >> 0x10),uVar24) & 0xffffffff;
        if (uVar18 < 2) {
          uVar19 = (ulong)((uint)uVar17 - 1) & 7;
        }
        else {
          uVar19 = 7;
          if (uVar17 < 8) {
            uVar2 = (uint)uVar17 & 0xff;
            uVar4 = 0;
            if ((uVar17 & 0xff) != 0) {
              uVar4 = 7 / uVar2;
            }
            uVar19 = (ulong)(7 - uVar4 * uVar2);
          }
        }
        plVar20 = *(long **)(*(long *)this_00 + uVar19 * 8);
        if ((plVar20 != (long *)0x0) && (plVar20 = (long *)*plVar20, plVar20 != (long *)0x0)) {
          do {
            uVar21 = plVar20[1];
            if (uVar21 == 7) {
              if ((int)plVar20[2] == 7) {
                local_90 = (void *)CONCAT44(local_90._4_4_,7);
                std::
                __hash_table<PipelineControlBase::Feature,std::hash<PipelineControlBase::Feature>,std::equal_to<PipelineControlBase::Feature>,std::allocator<PipelineControlBase::Feature>>
                ::__erase_unique<PipelineControlBase::Feature>(this_00,(Feature *)&local_90);
                break;
              }
            }
            else {
              if (uVar18 < 2) {
                uVar21 = uVar21 & uVar17 - 1;
              }
              else if (uVar17 <= uVar21) {
                uVar5 = 0;
                if (uVar17 != 0) {
                  uVar5 = uVar21 / uVar17;
                }
                uVar21 = uVar21 - uVar5 * uVar17;
              }
              if (uVar21 != uVar19) break;
            }
            plVar20 = (long *)*plVar20;
          } while (plVar20 != (long *)0x0);
        }
      }
      bVar9 = (uVar13 & 0xff) != 0;
      pXVar1 = this + 0x209;
      pXVar3 = pXVar1;
      if (((byte)this[0x208] & 1) != 0) {
        pXVar3 = *(XrCameraConfiguration **)(this + 0x218);
      }
      CameraHal::printLog(4,"XrCameraHal",
                          "%s:%s: %s ultra lite enabled/disable(1/0): %d for camera %u",
                          "XrCameraConfiguration.cpp",
                          "overrideMonoCameraConfigurationFromSystemProperties",pXVar3,bVar9,
                          *(undefined4 *)(this + 8));
      featureOverride(this,0xb,bVar9);
      local_70 = 0;
      uStack_88 = 0;
      local_90 = (void *)0x0;
      uStack_78 = 0;
      local_80 = (undefined8 *)0x0;
      std::mutex::lock();
      std::
      unordered_set<OVR::Peripherals::Config::Models::StreamType,std::hash<OVR::Peripherals::Config::Models::StreamType>,std::equal_to<OVR::Peripherals::Config::Models::StreamType>,std::allocator<OVR::Peripherals::Config::Models::StreamType>>
      ::unordered_set((unordered_set<OVR::Peripherals::Config::Models::StreamType,std::hash<OVR::Peripherals::Config::Models::StreamType>,std::equal_to<OVR::Peripherals::Config::Models::StreamType>,std::allocator<OVR::Peripherals::Config::Models::StreamType>>
                       *)&local_90,(unordered_set *)(this + 0x1b0));
      std::mutex::unlock();
      local_a0 = (piecewise_construct_t *)(this + 0x90);
      lVar15 = std::
               __hash_table<std::__hash_value_type<OVR::Peripherals::Config::Models::CameraHalUsecasePipelineType,std::vector<CameraHal::XrStreamConfiguration,std::allocator<CameraHal::XrStreamConfiguration>>>,std::__unordered_map_hasher<OVR::Peripherals::Config::Models::CameraHalUsecasePipelineType,std::__hash_value_type<OVR::Peripherals::Config::Models::CameraHalUsecasePipelineType,std::vector<CameraHal::XrStreamConfiguration,std::allocator<CameraHal::XrStreamConfiguration>>>,std::hash<OVR::Peripherals::Config::Models::CameraHalUsecasePipelineType>,true>,std::__unordered_map_equal<OVR::Peripherals::Config::Models::CameraHalUsecasePipelineType,std::__hash_value_type<OVR::Peripherals::Config::Models::CameraHalUsecasePipelineType,std::vector<CameraHal::XrStreamConfiguration,std::allocator<CameraHal::XrStreamConfiguration>>>,std::equal_to<OVR::Peripherals::Config::Models::CameraHalUsecasePipelineType>,true>,std::allocator<std::__hash_value_type<OVR::Peripherals::Config::Models::CameraHalUsecasePipelineType,std::vector<CameraHal::XrStreamConfiguration,std::allocator<CameraHal::XrStreamConfiguration>>>>>
               ::
               __emplace_unique_key_args<OVR::Peripherals::Config::Models::CameraHalUsecasePipelineType,std::piecewise_construct_t_const&,std::tuple<OVR::Peripherals::Config::Models::CameraHalUsecasePipelineType_const&>,std::tuple<>>
                         ((CameraHalUsecasePipelineType *)(this + 0x68),local_a0,
                          (tuple *)&DAT_001203c8,(tuple *)&local_a0);
      lVar23 = *(long *)(lVar15 + 0x18);
      lVar15 = *(long *)(lVar15 + 0x20);
      pvVar7 = local_90;
      puVar8 = local_80;
      if (lVar23 != lVar15) {
        do {
          uVar17 = OVR::OS::isWorldTrackingCamera(*(undefined4 *)(this + 8));
          if (((uVar11 & 0xff) != 0) && ((uVar17 & 1) != 0)) {
            pXVar3 = pXVar1;
            if (((byte)this[0x208] & 1) != 0) {
              pXVar3 = *(XrCameraConfiguration **)(this + 0x218);
            }
            CameraHal::printLog(5,"XrCameraHal",
                                "%s:%s: %s replacing 10 bits mono with 8 bits mono for camera %u",
                                "XrCameraConfiguration.cpp",
                                "overrideMonoCameraConfigurationFromSystemProperties",pXVar3,
                                *(undefined4 *)(this + 8));
            __s = *(char **)(lVar23 + 0x30);
            sVar16 = strlen(__s);
            local_a0 = apStack_a8;
            uVar17 = magic_enum::detail::
                     constexpr_switch<&(magic_enum::detail::names_v<OVR::Peripherals::Config::Models::StreamType>),(magic_enum::detail::case_call_t)0,0ul,magic_enum::detail::constexpr_hash_t<std::basic_string_view<char,std::char_traits<char>>,void>,magic_enum::enum_cast<OVR::Peripherals::Config::Models::StreamType,std::equal_to<void>>(std::basic_string_view<char,std::char_traits<char>>,std::equal_to<void>&&)::_lambda(unsigned_long)_1_,magic_enum::detail::default_result_type_lambda<std::optional<OVR::Peripherals::Config::Models::StreamType>>::_lambda()_1_const&,magic_enum::enum_cast<OVR::Peripherals::Config::Models::StreamType,std::equal_to<void>>(std::basic_string_view<char,std::char_traits<char>>,std::equal_to<void>&&)::_lambda(std::basic_string_view<char,std::char_traits<char>>,std::basic_string_view<char,std::char_traits<char>>)_1_>
                               (auStack_98,__s,sVar16,
                                PTR_default_result_type_lambda<std::optional<OVR::Peripherals::Config::Models::StreamType>>_00167140
                                ,&local_a0);
            uVar14 = 0;
            if ((uVar17 & 0xff00000000) != 0) {
              uVar14 = (undefined4)uVar17;
            }
            local_a0._0_4_ = uVar14;
            std::
            __hash_table<OVR::Peripherals::Config::Models::StreamType,std::hash<OVR::Peripherals::Config::Models::StreamType>,std::equal_to<OVR::Peripherals::Config::Models::StreamType>,std::allocator<OVR::Peripherals::Config::Models::StreamType>>
            ::__erase_unique<OVR::Peripherals::Config::Models::StreamType>
                      ((__hash_table<OVR::Peripherals::Config::Models::StreamType,std::hash<OVR::Peripherals::Config::Models::StreamType>,std::equal_to<OVR::Peripherals::Config::Models::StreamType>,std::allocator<OVR::Peripherals::Config::Models::StreamType>>
                        *)&local_90,(StreamType *)&local_a0);
            if (*(char *)(*(long *)this + 0x40) == '\0') {
              uVar24 = 1;
            }
            else {
              uVar24 = 0xd;
              if (*(int *)(*(long *)this + 4) != 0) {
                uVar24 = 0x11;
              }
            }
            *(undefined2 *)(lVar23 + 8) = uVar24;
            *(undefined **)(lVar23 + 0x30) =
                 PTR_enum_name_v<OVR::Peripherals::Config::Models::StreamType,(OVR::Peripherals::Config::Models::StreamType)3>_00167148
            ;
            *(undefined1 *)(lVar23 + 0x38) = 1;
            featureOverride(this,9,1);
            local_a0 = (piecewise_construct_t *)CONCAT44(local_a0._4_4_,3);
            std::
            __hash_table<OVR::Peripherals::Config::Models::StreamType,std::hash<OVR::Peripherals::Config::Models::StreamType>,std::equal_to<OVR::Peripherals::Config::Models::StreamType>,std::allocator<OVR::Peripherals::Config::Models::StreamType>>
            ::
            __emplace_unique_key_args<OVR::Peripherals::Config::Models::StreamType,OVR::Peripherals::Config::Models::StreamType>
                      ((__hash_table<OVR::Peripherals::Config::Models::StreamType,std::hash<OVR::Peripherals::Config::Models::StreamType>,std::equal_to<OVR::Peripherals::Config::Models::StreamType>,std::allocator<OVR::Peripherals::Config::Models::StreamType>>
                        *)&local_90,(StreamType *)&local_a0,(StreamType *)&local_a0);
          }
          if ((uVar12 & 0xff) != 0) {
            pXVar3 = pXVar1;
            if (((byte)this[0x208] & 1) != 0) {
              pXVar3 = *(XrCameraConfiguration **)(this + 0x218);
            }
            CameraHal::printLog(4,"XrCameraHal",
                                "%s:%s: %s replacing non-secure camera with secure camera for camera %u"
                                ,"XrCameraConfiguration.cpp",
                                "overrideMonoCameraConfigurationFromSystemProperties",pXVar3,
                                *(undefined4 *)(this + 8));
            *(undefined4 *)(lVar23 + 0x3c) = 0x4000;
            featureOverride(this,8,1);
          }
          lVar23 = lVar23 + 0x48;
          pvVar7 = local_90;
          puVar8 = local_80;
        } while (lVar23 != lVar15);
      }
      while (puVar8 != (void *)0x0) {
        pvVar22 = (void *)*puVar8;
        local_90 = pvVar7;
        operator_delete(puVar8);
        pvVar7 = local_90;
        puVar8 = pvVar22;
      }
      local_90 = (void *)0x0;
      if (pvVar7 != (void *)0x0) {
        operator_delete(pvVar7);
      }
    }
    else {
      local_90 = (void *)CONCAT44(local_90._4_4_,7);
      std::
      __hash_table<PipelineControlBase::Feature,std::hash<PipelineControlBase::Feature>,std::equal_to<PipelineControlBase::Feature>,std::allocator<PipelineControlBase::Feature>>
      ::__emplace_unique_key_args<PipelineControlBase::Feature,PipelineControlBase::Feature>
                (this_00,(Feature *)&local_90,(Feature *)&local_90);
    }
  }
  if (*(long *)(lVar6 + 0x28) == local_68) {
    return;
  }
                    /* WARNING: Subroutine does not return */
  __stack_chk_fail();
}



====================================================================================================
// overrideBayerFeatureSetFromSystemProperties  @ 001348d0

/* CameraHal::XrCameraConfiguration::overrideBayerFeatureSetFromSystemProperties() */

void __thiscall
CameraHal::XrCameraConfiguration::overrideBayerFeatureSetFromSystemProperties
          (XrCameraConfiguration *this)

{
  bool bVar1;
  XrCameraConfiguration *pXVar2;
  XrCameraConfiguration *pXVar3;
  bool bVar4;
  bool bVar5;
  bool bVar6;
  bool bVar7;
  bool bVar8;
  ushort uVar9;
  ushort uVar10;
  ushort uVar11;
  ushort uVar12;
  ushort uVar13;
  int iVar14;
  ulong uVar15;
  
  if (*(char *)(*(long *)this + 0x40) == '\0') {
    return;
  }
  uVar9 = oculus::sysprop::XrtHalQcProperties::teton_output_yuv();
  uVar10 = oculus::sysprop::XrtHalQcProperties::rgb_enable_3a();
  uVar11 = oculus::sysprop::XrtHalQcProperties::teton_enable_gme_mctf();
  uVar12 = oculus::sysprop::XrtHalQcProperties::teton_enable_fastai();
  uVar13 = oculus::sysprop::VrosVendorSessionlessProperties::oculus_camera_enable_rgb_crop();
  if (uVar13 < 0x101) {
    uVar13 = 0;
  }
  uVar15 = oculus::sysprop::XrtHalQcProperties::isp_foveation_mode();
  iVar14 = 0;
  if ((uVar15 & 0xff00000000) != 0) {
    iVar14 = (int)uVar15;
  }
  bVar1 = iVar14 != 0 && iVar14 < 0x16;
  if (((uVar13 & 0xff) != 0) && (bVar1)) {
                    /* WARNING: Subroutine does not return */
    __android_log_assert
              ("enableFoveation && enableCroppedStream","XrCameraHal",
               "Cannot enable both foveation and strema cropping");
  }
  bVar4 = (uVar13 & 0xff) != 0;
  if (uVar12 < 0x101) {
    uVar12 = 0;
  }
  bVar5 = (uVar12 & 0xff) != 0;
  if (uVar11 < 0x101) {
    uVar11 = 0;
  }
  bVar6 = (uVar11 & 0xff) != 0;
  if (uVar10 < 0x101) {
    uVar10 = 0;
  }
  bVar7 = (uVar10 & 0xff) != 0;
  if (uVar9 < 0x101) {
    uVar9 = 0;
  }
  pXVar2 = this + 0x209;
  bVar8 = (uVar9 & 0xff) != 0;
  pXVar3 = pXVar2;
  if (((byte)this[0x208] & 1) != 0) {
    pXVar3 = *(XrCameraConfiguration **)(this + 0x218);
  }
  CameraHal::printLog(4,"XrCameraHal","%s:%s: %s YUV output enabled/disable(1/0): %d for camera %u",
                      "XrCameraConfiguration.cpp","overrideBayerFeatureSetFromSystemProperties",
                      pXVar3,bVar8,*(undefined4 *)(this + 8));
  featureOverride(this,1,bVar8);
  pXVar3 = pXVar2;
  if (((byte)this[0x208] & 1) != 0) {
    pXVar3 = *(XrCameraConfiguration **)(this + 0x218);
  }
  CameraHal::printLog(4,"XrCameraHal",
                      "%s:%s: %s temporal filtering enabled/disable(1/0): %d for camera %u",
                      "XrCameraConfiguration.cpp","overrideBayerFeatureSetFromSystemProperties",
                      pXVar3,bVar6,*(undefined4 *)(this + 8));
  featureOverride(this,4,bVar6);
  pXVar3 = pXVar2;
  if (((byte)this[0x208] & 1) != 0) {
    pXVar3 = *(XrCameraConfiguration **)(this + 0x218);
  }
  CameraHal::printLog(4,"XrCameraHal","%s:%s: %s 3A control enabled/disable(1/0): %d for camera %u",
                      "XrCameraConfiguration.cpp","overrideBayerFeatureSetFromSystemProperties",
                      pXVar3,bVar7,*(undefined4 *)(this + 8));
  featureOverride(this,2,bVar7);
  featureOverride(this,0,bVar7);
  featureOverride(this,3,bVar7);
  pXVar3 = pXVar2;
  if (((byte)this[0x208] & 1) != 0) {
    pXVar3 = *(XrCameraConfiguration **)(this + 0x218);
  }
  CameraHal::printLog(4,"XrCameraHal","%s:%s: %s CoreAi enabled/disable(1/0): %d for camera %u",
                      "XrCameraConfiguration.cpp","overrideBayerFeatureSetFromSystemProperties",
                      pXVar3,bVar5,*(undefined4 *)(this + 8));
  featureOverride(this,5,bVar5);
  pXVar3 = pXVar2;
  if (((byte)this[0x208] & 1) != 0) {
    pXVar3 = *(XrCameraConfiguration **)(this + 0x218);
  }
  CameraHal::printLog(4,"XrCameraHal",
                      "%s:%s: %s stream cropping enabled/disable(1/0): %d for camera %u",
                      "XrCameraConfiguration.cpp","overrideBayerFeatureSetFromSystemProperties",
                      pXVar3,bVar4,*(undefined4 *)(this + 8));
  featureOverride(this,0xc,bVar4);
  if (((byte)this[0x208] & 1) != 0) {
    pXVar2 = *(XrCameraConfiguration **)(this + 0x218);
  }
  CameraHal::printLog(4,"XrCameraHal","%s:%s: %s foveation enabled/disable(1/0): %d for camera %u",
                      "XrCameraConfiguration.cpp","overrideBayerFeatureSetFromSystemProperties",
                      pXVar2,bVar1,*(undefined4 *)(this + 8));
  featureOverride(this,6,bVar1);
  return;
}



====================================================================================================
// onGetCameraType  @ 0013ef20

/* CameraHal::XrCameraLogical::onGetCameraType(unsigned int) */

void __thiscall CameraHal::XrCameraLogical::onGetCameraType(XrCameraLogical *this,uint param_1)

{
  long lVar1;
  long lVar2;
  uint local_3c [3];
  uint *local_30;
  long local_28;
  
  lVar1 = tpidr_el0;
  local_28 = *(long *)(lVar1 + 0x28);
  local_30 = local_3c;
  local_3c[0] = param_1;
  lVar2 = std::
          __hash_table<std::__hash_value_type<unsigned_int,std::shared_ptr<CameraHal::XrCameraConfiguration>>,std::__unordered_map_hasher<unsigned_int,std::__hash_value_type<unsigned_int,std::shared_ptr<CameraHal::XrCameraConfiguration>>,std::hash<unsigned_int>,true>,std::__unordered_map_equal<unsigned_int,std::__hash_value_type<unsigned_int,std::shared_ptr<CameraHal::XrCameraConfiguration>>,std::equal_to<unsigned_int>,true>,std::allocator<std::__hash_value_type<unsigned_int,std::shared_ptr<CameraHal::XrCameraConfiguration>>>>
          ::
          __emplace_unique_key_args<unsigned_int,std::piecewise_construct_t_const&,std::tuple<unsigned_int_const&>,std::tuple<>>
                    ((uint *)(this + 0x220),(piecewise_construct_t *)local_3c,(tuple *)&DAT_001204d0
                     ,(tuple *)&local_30);
  if (*(long *)(lVar1 + 0x28) == local_28) {
    return;
  }
                    /* WARNING: Subroutine does not return */
  __stack_chk_fail(*(undefined4 *)**(undefined8 **)(lVar2 + 0x18));
}



====================================================================================================
// onIsBayerCamera  @ 00144780

/* CameraHal::XrCameraLogical::onIsBayerCamera(unsigned int) */

undefined1 __thiscall
CameraHal::XrCameraLogical::onIsBayerCamera(XrCameraLogical *this,uint param_1)

{
  ulong uVar1;
  ulong uVar2;
  long *plVar3;
  ulong uVar4;
  undefined2 uVar5;
  undefined8 uVar6;
  
  uVar2 = *(ulong *)(this + 0x228);
  uVar1 = (ulong)param_1;
  uVar6 = CONCAT17(POPCOUNT((char)(uVar2 >> 0x38)),
                   CONCAT16(POPCOUNT((char)(uVar2 >> 0x30)),
                            CONCAT15(POPCOUNT((char)(uVar2 >> 0x28)),
                                     CONCAT14(POPCOUNT((char)(uVar2 >> 0x20)),
                                              CONCAT13(POPCOUNT((char)(uVar2 >> 0x18)),
                                                       CONCAT12(POPCOUNT((char)(uVar2 >> 0x10)),
                                                                CONCAT11(POPCOUNT((char)(uVar2 >> 8)
                                                                                 ),
                                                                         POPCOUNT((char)uVar2)))))))
                  );
  uVar5 = NEON_uaddlv(uVar6,1);
  if ((uint)CONCAT62((int6)((ulong)uVar6 >> 0x10),uVar5) < 2) {
    uVar4 = (ulong)((int)uVar2 - 1U & param_1);
  }
  else {
    uVar4 = uVar1;
    if (uVar2 <= uVar1) {
      uVar4 = 0;
      if (uVar2 != 0) {
        uVar4 = uVar1 / uVar2;
      }
      uVar4 = uVar1 - uVar4 * uVar2;
    }
  }
  plVar3 = *(long **)(*(long *)(this + 0x220) + uVar4 * 8);
  do {
    do {
      plVar3 = (long *)*plVar3;
    } while (plVar3[1] != uVar1);
  } while (*(uint *)(plVar3 + 2) != param_1);
  return *(undefined1 *)(*(long *)plVar3[3] + 0x40);
}



====================================================================================================
// onGetEnabledStreamTypeList  @ 00144cc0

/* CameraHal::XrCameraLogical::onGetEnabledStreamTypeList(unsigned int) */

void CameraHal::XrCameraLogical::onGetEnabledStreamTypeList(uint param_1)

{
  uint in_w1;
  unordered_set<OVR::Peripherals::Config::Models::StreamType,std::hash<OVR::Peripherals::Config::Models::StreamType>,std::equal_to<OVR::Peripherals::Config::Models::StreamType>,std::allocator<OVR::Peripherals::Config::Models::StreamType>>
  *in_x8;
  ulong uVar1;
  long lVar2;
  ulong uVar3;
  long *plVar4;
  ulong uVar5;
  undefined2 uVar6;
  undefined8 uVar7;
  
  uVar3 = *(ulong *)((ulong)param_1 + 0x228);
  uVar1 = (ulong)in_w1;
  uVar7 = CONCAT17(POPCOUNT((char)(uVar3 >> 0x38)),
                   CONCAT16(POPCOUNT((char)(uVar3 >> 0x30)),
                            CONCAT15(POPCOUNT((char)(uVar3 >> 0x28)),
                                     CONCAT14(POPCOUNT((char)(uVar3 >> 0x20)),
                                              CONCAT13(POPCOUNT((char)(uVar3 >> 0x18)),
                                                       CONCAT12(POPCOUNT((char)(uVar3 >> 0x10)),
                                                                CONCAT11(POPCOUNT((char)(uVar3 >> 8)
                                                                                 ),
                                                                         POPCOUNT((char)uVar3)))))))
                  );
  uVar6 = NEON_uaddlv(uVar7,1);
  if ((uint)CONCAT62((int6)((ulong)uVar7 >> 0x10),uVar6) < 2) {
    uVar5 = (ulong)((int)uVar3 - 1U & in_w1);
  }
  else {
    uVar5 = uVar1;
    if (uVar3 <= uVar1) {
      uVar5 = 0;
      if (uVar3 != 0) {
        uVar5 = uVar1 / uVar3;
      }
      uVar5 = uVar1 - uVar5 * uVar3;
    }
  }
  plVar4 = *(long **)(*(long *)((ulong)param_1 + 0x220) + uVar5 * 8);
  do {
    do {
      plVar4 = (long *)*plVar4;
    } while (plVar4[1] != uVar1);
  } while (*(uint *)(plVar4 + 2) != in_w1);
  lVar2 = plVar4[3];
  std::mutex::lock();
  std::mutex::unlock();
  std::
  unordered_set<OVR::Peripherals::Config::Models::StreamType,std::hash<OVR::Peripherals::Config::Models::StreamType>,std::equal_to<OVR::Peripherals::Config::Models::StreamType>,std::allocator<OVR::Peripherals::Config::Models::StreamType>>
  ::unordered_set(in_x8,(unordered_set *)(lVar2 + 0x1b0));
  return;
}



====================================================================================================
// GetSupportedCameraIds  @ 0014ee70

/* CameraHal::XrCameraHal::GetSupportedCameraIds() */

void CameraHal::XrCameraHal::GetSupportedCameraIds(void)

{
  void *__src;
  ulong __n;
  long in_x0;
  void *__dest;
  undefined8 *in_x8;
  long lVar1;
  
  lVar1 = *(long *)(in_x0 + 0x90);
  *in_x8 = 0;
  in_x8[1] = 0;
  in_x8[2] = 0;
  __src = *(void **)(lVar1 + 0x30);
  __n = *(long *)(lVar1 + 0x38) - (long)__src;
  if (__n != 0) {
    if ((long)__n < 0) {
      lVar1 = std::__vector_base_common<true>::__throw_length_error();
      XrCameraManager::getCamera((uint)*(undefined8 *)(lVar1 + 0x90));
      return;
    }
    __dest = operator_new(__n);
    *in_x8 = __dest;
    in_x8[2] = (void *)((long)__dest + ((long)__n >> 2) * 4);
    memcpy(__dest,__src,__n);
    in_x8[1] = (long)__dest + __n;
  }
  return;
}



