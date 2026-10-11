// C:\Users\remus\quest3-ir-passthrough\ota-inspect\provider\virtual_camera
// functions referencing strings: ['cameraSource PASSTHROUGH or AVATAR', 'not allowed to set cameraId', 'one of these permissions']

====================================================================================================
// unregisterCamera @ 00132cc0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */
/* android::companion::virtualcamera::VirtualCameraService::unregisterCamera(ndk::SpAIBinder const&)
    */

void android::companion::virtualcamera::VirtualCameraService::unregisterCamera(SpAIBinder *param_1)

{
  ulong uVar1;
  long lVar2;
  undefined8 uVar3;
  undefined8 uVar4;
  undefined8 uVar5;
  void *pvVar6;
  uint uVar7;
  undefined4 uVar8;
  undefined4 uVar9;
  ulong uVar10;
  undefined8 uVar11;
  ulong *in_x1;
  undefined8 *in_x8;
  ulong uVar12;
  long *plVar13;
  ulong uVar14;
  ulong uVar15;
  long *plVar16;
  ulong uVar17;
  ulong uVar18;
  long *plVar19;
  VirtualCameraProvider *this;
  undefined8 *puVar20;
  undefined **ppuVar21;
  long lVar22;
  undefined2 uVar23;
  ulong local_90;
  undefined8 uStack_88;
  char *local_80;
  void *local_70;
  undefined8 uStack_68;
  char *local_60;
  long local_58;
  
  lVar2 = tpidr_el0;
  local_58 = *(long *)(lVar2 + 0x28);
  puVar20 = *(undefined8 **)(param_1 + 0x68);
  local_60 = operator_new(0x30);
  uStack_68 = _UNK_001091b8;
  local_70 = _DAT_001091b0;
  local_60[0x28] = '\0';
  uVar5 = s_android_permission_CREATE_VIRTUA_0010df58._24_8_;
  uVar4 = s_android_permission_CREATE_VIRTUA_0010df58._16_8_;
  uVar3 = s_android_permission_CREATE_VIRTUA_0010df58._8_8_;
  uVar11 = s_android_permission_CREATE_VIRTUA_0010df58._0_8_;
  *(undefined8 *)(local_60 + 0x20) = s_android_permission_CREATE_VIRTUA_0010df58._32_8_;
  *(undefined8 *)(local_60 + 8) = uVar3;
  *(undefined8 *)local_60 = uVar11;
  *(undefined8 *)(local_60 + 0x18) = uVar5;
  *(undefined8 *)(local_60 + 0x10) = uVar4;
  ppuVar21 = (undefined **)*puVar20;
  if (ppuVar21 != &PTR____node_00175c20) {
    __cfi_slowpath(0x293295f4e1fa4e55,ppuVar21);
  }
  uVar10 = (*(code *)ppuVar21[2])(puVar20,&local_70);
  if ((uVar10 & 1) == 0) {
    puVar20 = *(undefined8 **)(param_1 + 0x68);
    local_80 = operator_new(0x30);
    uStack_88 = _UNK_00109318;
    local_90 = _DAT_00109310;
    local_80[0x2a] = '\0';
    uVar4 = s_horizonos_permission_CREATE_VIRT_0010df81._16_8_;
    uVar3 = s_horizonos_permission_CREATE_VIRT_0010df81._0_8_;
    uVar11 = CONCAT62(s_horizonos_permission_CREATE_VIRT_0010df81._26_6_,
                      s_horizonos_permission_CREATE_VIRT_0010df81._24_2_);
    *(undefined8 *)(local_80 + 8) = s_horizonos_permission_CREATE_VIRT_0010df81._8_8_;
    *(undefined8 *)local_80 = uVar3;
    *(undefined8 *)(local_80 + 0x18) = uVar11;
    *(undefined8 *)(local_80 + 0x10) = uVar4;
    uVar11 = CONCAT26(s_horizonos_permission_CREATE_VIRT_0010df81._32_2_,
                      s_horizonos_permission_CREATE_VIRT_0010df81._26_6_);
    *(undefined8 *)(local_80 + 0x22) = s_horizonos_permission_CREATE_VIRT_0010df81._34_8_;
    *(undefined8 *)(local_80 + 0x1a) = uVar11;
    ppuVar21 = (undefined **)*puVar20;
    if (ppuVar21 != &PTR____node_00175c20) {
      __cfi_slowpath(0x293295f4e1fa4e55,ppuVar21);
    }
    uVar7 = (*(code *)ppuVar21[2])(puVar20,&local_90);
    uVar7 = uVar7 ^ 1;
    if ((local_90 & 1) != 0) {
      operator_delete(local_80);
    }
  }
  else {
    uVar7 = 0;
  }
  if (((ulong)local_70 & 1) != 0) {
    operator_delete(local_60);
  }
  if ((uVar7 & 1) == 0) {
    std::mutex::lock();
    uVar10 = *in_x1;
    uVar12 = *(ulong *)(param_1 + 0xa0);
    if (uVar12 != 0) {
      uVar11 = CONCAT17(POPCOUNT((char)(uVar12 >> 0x38)),
                        CONCAT16(POPCOUNT((char)(uVar12 >> 0x30)),
                                 CONCAT15(POPCOUNT((char)(uVar12 >> 0x28)),
                                          CONCAT14(POPCOUNT((char)(uVar12 >> 0x20)),
                                                   CONCAT13(POPCOUNT((char)(uVar12 >> 0x18)),
                                                            CONCAT12(POPCOUNT((char)(uVar12 >> 0x10)
                                                                             ),
                                                                     CONCAT11(POPCOUNT((char)(uVar12
                                                                                             >> 8)),
                                                                              POPCOUNT((char)uVar12)
                                                                             )))))));
      uVar14 = ((ulong)(uint)((int)uVar10 << 3) + 8 ^ uVar10 >> 0x20) * -0x622015f714c7d297;
      uVar23 = NEON_uaddlv(uVar11,1);
      uVar14 = (uVar10 >> 0x20 ^ uVar14 >> 0x2f ^ uVar14) * -0x622015f714c7d297;
      uVar15 = CONCAT62((int6)((ulong)uVar11 >> 0x10),uVar23) & 0xffffffff;
      uVar14 = (uVar14 ^ uVar14 >> 0x2f) * -0x622015f714c7d297;
      if (uVar15 < 2) {
        uVar17 = uVar14 & uVar12 - 1;
      }
      else {
        uVar17 = uVar14;
        if (uVar12 <= uVar14) {
          uVar17 = 0;
          if (uVar12 != 0) {
            uVar17 = uVar14 / uVar12;
          }
          uVar17 = uVar14 - uVar17 * uVar12;
        }
      }
      plVar16 = *(long **)(*(long *)(param_1 + 0x98) + uVar17 * 8);
      if ((plVar16 != (long *)0x0) && (plVar16 = (long *)*plVar16, plVar16 != (long *)0x0)) {
LAB_00132f14:
        uVar18 = plVar16[1];
        if (uVar18 == uVar14) {
          if (plVar16[2] != uVar10) goto LAB_00132f0c;
          for (plVar13 = *(long **)(param_1 + 0xd0); plVar13 != (long *)0x0;
              plVar13 = (long *)*plVar13) {
            uVar12 = plVar13[4];
            if (uVar12 != 0) {
              uVar11 = CONCAT17(POPCOUNT((char)(uVar12 >> 0x38)),
                                CONCAT16(POPCOUNT((char)(uVar12 >> 0x30)),
                                         CONCAT15(POPCOUNT((char)(uVar12 >> 0x28)),
                                                  CONCAT14(POPCOUNT((char)(uVar12 >> 0x20)),
                                                           CONCAT13(POPCOUNT((char)(uVar12 >> 0x18))
                                                                    ,CONCAT12(POPCOUNT((char)(uVar12
                                                                                             >> 0x10
                                                  )),CONCAT11(POPCOUNT((char)(uVar12 >> 8)),
                                                              POPCOUNT((char)uVar12))))))));
              uVar23 = NEON_uaddlv(uVar11,1);
              uVar15 = CONCAT62((int6)((ulong)uVar11 >> 0x10),uVar23) & 0xffffffff;
              if (uVar15 < 2) {
                uVar17 = uVar14 & uVar12 - 1;
              }
              else {
                uVar17 = uVar14;
                if (uVar12 <= uVar14) {
                  uVar17 = 0;
                  if (uVar12 != 0) {
                    uVar17 = uVar14 / uVar12;
                  }
                  uVar17 = uVar14 - uVar17 * uVar12;
                }
              }
              plVar19 = *(long **)(plVar13[3] + uVar17 * 8);
              if ((plVar19 != (long *)0x0) && (plVar19 = (long *)*plVar19, plVar19 != (long *)0x0))
              {
                do {
                  uVar18 = plVar19[1];
                  if (uVar18 == uVar14) {
                    if (plVar19[2] == uVar10) {
                      std::
                      __hash_table<ndk::SpAIBinder,android::companion::virtualcamera::VirtualCameraService::BinderTokenHash,std::equal_to<ndk::SpAIBinder>,std::allocator<ndk::SpAIBinder>>
                      ::remove(&local_70);
                      pvVar6 = local_70;
                      local_70 = (void *)0x0;
                      if (pvVar6 != (void *)0x0) {
                        if ((char)local_60 != '\0') {
                          lVar22 = *(long *)((long)pvVar6 + 0x10);
                          if (lVar22 != 0) {
                            AIBinder_decStrong(lVar22);
                          }
                          if (lVar22 != *(long *)((long)pvVar6 + 0x10)) {
                    /* WARNING: Subroutine does not return */
                            __assert("frameworks/native/libs/binder/ndk/include_cpp/android/binder_auto_utils.h"
                                     ,(char *)0x5d,0x10beea);
                          }
                        }
                        operator_delete(pvVar6);
                      }
                      goto LAB_001330bc;
                    }
                  }
                  else {
                    if (uVar15 < 2) {
                      uVar18 = uVar18 & uVar12 - 1;
                    }
                    else if (uVar12 <= uVar18) {
                      uVar1 = 0;
                      if (uVar12 != 0) {
                        uVar1 = uVar18 / uVar12;
                      }
                      uVar18 = uVar18 - uVar1 * uVar12;
                    }
                    if (uVar18 != uVar17) break;
                  }
                  plVar19 = (long *)*plVar19;
                } while (plVar19 != (long *)0x0);
              }
            }
          }
LAB_001330bc:
          this = *(VirtualCameraProvider **)(param_1 + 0x50);
          if (*(undefined ***)this != &PTR__VirtualCameraProvider_00176040) {
            __cfi_slowpath(0x56f3385870b5b2d3);
          }
          VirtualCameraProvider::removeCamera(this,(string *)(plVar16 + 3));
          std::
          __hash_table<std::__hash_value_type<ndk::SpAIBinder,std::string>,std::__unordered_map_hasher<ndk::SpAIBinder,std::__hash_value_type<ndk::SpAIBinder,std::string>,android::companion::virtualcamera::VirtualCameraService::BinderTokenHash,true>,std::__unordered_map_equal<ndk::SpAIBinder,std::__hash_value_type<ndk::SpAIBinder,std::string>,std::equal_to<ndk::SpAIBinder>,true>,std::allocator<std::__hash_value_type<ndk::SpAIBinder,std::string>>>
          ::erase((__hash_table<std::__hash_value_type<ndk::SpAIBinder,std::string>,std::__unordered_map_hasher<ndk::SpAIBinder,std::__hash_value_type<ndk::SpAIBinder,std::string>,android::companion::virtualcamera::VirtualCameraService::BinderTokenHash,true>,std::__unordered_map_equal<ndk::SpAIBinder,std::__hash_value_type<ndk::SpAIBinder,std::string>,std::equal_to<ndk::SpAIBinder>,true>,std::allocator<std::__hash_value_type<ndk::SpAIBinder,std::string>>>
                   *)(param_1 + 0x98),plVar16);
          goto LAB_00132f70;
        }
        if (uVar15 < 2) {
          uVar18 = uVar18 & uVar12 - 1;
        }
        else if (uVar12 <= uVar18) {
          uVar1 = 0;
          if (uVar12 != 0) {
            uVar1 = uVar18 / uVar12;
          }
          uVar18 = uVar18 - uVar1 * uVar12;
        }
        if (uVar18 == uVar17) goto LAB_00132f0c;
      }
    }
LAB_00132f58:
    __android_log_print(6,"VirtualCameraService",
                        "Attempt to unregister camera corresponding to unknown binder token: 0x%lx")
    ;
LAB_00132f70:
    uVar11 = AStatus_newOk();
    *in_x8 = uVar11;
    std::mutex::unlock();
  }
  else {
    android::IPCThreadState::self();
    uVar8 = android::IPCThreadState::getCallingPid();
    android::IPCThreadState::self();
    uVar9 = android::IPCThreadState::getCallingUid();
    __android_log_print(6,"VirtualCameraService",
                        "%s: caller (pid %d, uid %d) doesn\'t hold one of these permissions: %s, %s"
                        ,"unregisterCamera",uVar8,uVar9,"android.permission.CREATE_VIRTUAL_DEVICE",
                        "horizonos.permission.CREATE_VIRTUAL_CAMERA");
    uVar11 = AStatus_fromExceptionCode(0xffffffff);
    *in_x8 = uVar11;
  }
  if (*(long *)(lVar2 + 0x28) != local_58) {
                    /* WARNING: Subroutine does not return */
    __stack_chk_fail();
  }
  return;
LAB_00132f0c:
  plVar16 = (long *)*plVar16;
  if (plVar16 == (long *)0x0) goto LAB_00132f58;
  goto LAB_00132f14;
}



====================================================================================================
// getCameraId @ 00133160

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */
/* android::companion::virtualcamera::VirtualCameraService::getCameraId(ndk::SpAIBinder const&,
   std::string*) */

void android::companion::virtualcamera::VirtualCameraService::getCameraId
               (SpAIBinder *param_1,string *param_2)

{
  long lVar1;
  undefined8 uVar2;
  undefined8 uVar3;
  undefined8 uVar4;
  long *plVar5;
  long lVar6;
  uint uVar7;
  undefined4 uVar8;
  undefined4 uVar9;
  ulong uVar10;
  undefined8 uVar11;
  string *in_x2;
  undefined8 *in_x8;
  undefined **ppuVar12;
  undefined8 *puVar13;
  ulong local_90;
  undefined8 uStack_88;
  char *local_80;
  long *local_70;
  long *plStack_68;
  char *local_60;
  long local_58;
  
  lVar1 = tpidr_el0;
  local_58 = *(long *)(lVar1 + 0x28);
  puVar13 = *(undefined8 **)(param_1 + 0x68);
  local_60 = operator_new(0x30);
  plStack_68 = _UNK_001091b8;
  local_70 = _DAT_001091b0;
  local_60[0x28] = '\0';
  uVar4 = s_android_permission_CREATE_VIRTUA_0010df58._24_8_;
  uVar3 = s_android_permission_CREATE_VIRTUA_0010df58._16_8_;
  uVar2 = s_android_permission_CREATE_VIRTUA_0010df58._8_8_;
  uVar11 = s_android_permission_CREATE_VIRTUA_0010df58._0_8_;
  *(undefined8 *)(local_60 + 0x20) = s_android_permission_CREATE_VIRTUA_0010df58._32_8_;
  *(undefined8 *)(local_60 + 8) = uVar2;
  *(undefined8 *)local_60 = uVar11;
  *(undefined8 *)(local_60 + 0x18) = uVar4;
  *(undefined8 *)(local_60 + 0x10) = uVar3;
  ppuVar12 = (undefined **)*puVar13;
  if (ppuVar12 != &PTR____node_00175c20) {
    __cfi_slowpath(0x293295f4e1fa4e55,ppuVar12);
  }
  uVar10 = (*(code *)ppuVar12[2])(puVar13,&local_70);
  if ((uVar10 & 1) == 0) {
    puVar13 = *(undefined8 **)(param_1 + 0x68);
    local_80 = operator_new(0x30);
    uStack_88 = _UNK_00109318;
    local_90 = _DAT_00109310;
    local_80[0x2a] = '\0';
    uVar3 = s_horizonos_permission_CREATE_VIRT_0010df81._16_8_;
    uVar2 = s_horizonos_permission_CREATE_VIRT_0010df81._0_8_;
    uVar11 = CONCAT62(s_horizonos_permission_CREATE_VIRT_0010df81._26_6_,
                      s_horizonos_permission_CREATE_VIRT_0010df81._24_2_);
    *(undefined8 *)(local_80 + 8) = s_horizonos_permission_CREATE_VIRT_0010df81._8_8_;
    *(undefined8 *)local_80 = uVar2;
    *(undefined8 *)(local_80 + 0x18) = uVar11;
    *(undefined8 *)(local_80 + 0x10) = uVar3;
    uVar11 = CONCAT26(s_horizonos_permission_CREATE_VIRT_0010df81._32_2_,
                      s_horizonos_permission_CREATE_VIRT_0010df81._26_6_);
    *(undefined8 *)(local_80 + 0x22) = s_horizonos_permission_CREATE_VIRT_0010df81._34_8_;
    *(undefined8 *)(local_80 + 0x1a) = uVar11;
    ppuVar12 = (undefined **)*puVar13;
    if (ppuVar12 != &PTR____node_00175c20) {
      __cfi_slowpath(0x293295f4e1fa4e55,ppuVar12);
    }
    uVar7 = (*(code *)ppuVar12[2])(puVar13,&local_90);
    uVar7 = uVar7 ^ 1;
    if ((local_90 & 1) != 0) {
      operator_delete(local_80);
    }
  }
  else {
    uVar7 = 0;
  }
  if (((ulong)local_70 & 1) != 0) {
    operator_delete(local_60);
  }
  if ((uVar7 & 1) == 0) {
    if (in_x2 == (string *)0x0) {
      uVar11 = AStatus_fromServiceSpecificError(0xfffffffd);
      *in_x8 = uVar11;
    }
    else {
      local_70 = (long *)0x0;
      plStack_68 = (long *)0x0;
      if (*(undefined ***)param_1 != &PTR__VirtualCameraService_00176640) {
        __cfi_slowpath(0x7fd7667d391a1952);
      }
      getCamera(param_1);
      plVar5 = local_70;
      if (local_70 == (long *)0x0) {
        __android_log_print(6,"VirtualCameraService",
                            "Attempt to get camera id corresponding to unknown binder token: 0x%lx",
                            *(undefined8 *)param_2);
      }
      else {
        if ((undefined **)*local_70 != &PTR__VirtualCameraDevice_00176220) {
          __cfi_slowpath(0x8d0ad1676903e960);
        }
        std::string::operator=(in_x2,(string *)(plVar5 + 10));
      }
      uVar11 = AStatus_newOk();
      *in_x8 = uVar11;
      if (plStack_68 != (long *)0x0) {
        LOAcquire();
        lVar6 = plStack_68[1];
        plStack_68[1] = lVar6 + -1;
        LORelease();
        if (lVar6 == 0) {
          (**(code **)(*plStack_68 + 0x10))(plStack_68);
          std::__shared_weak_count::__release_weak();
        }
      }
    }
  }
  else {
    android::IPCThreadState::self();
    uVar8 = android::IPCThreadState::getCallingPid();
    android::IPCThreadState::self();
    uVar9 = android::IPCThreadState::getCallingUid();
    __android_log_print(6,"VirtualCameraService",
                        "%s: caller (pid %d, uid %d) doesn\'t hold one of these permissions: %s, %s"
                        ,"getCameraId",uVar8,uVar9,"android.permission.CREATE_VIRTUAL_DEVICE",
                        "horizonos.permission.CREATE_VIRTUAL_CAMERA");
    uVar11 = AStatus_fromExceptionCode(0xffffffff);
    *in_x8 = uVar11;
  }
  if (*(long *)(lVar1 + 0x28) != local_58) {
                    /* WARNING: Subroutine does not return */
    __stack_chk_fail();
  }
  return;
}



====================================================================================================
// registerCamera @ 001323e0

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */
/* android::companion::virtualcamera::VirtualCameraService::registerCamera(ndk::SpAIBinder const&,
   aidl::android::companion::virtualcamera::VirtualCameraConfiguration const&, int, bool*) */

void android::companion::virtualcamera::VirtualCameraService::registerCamera
               (SpAIBinder *param_1,VirtualCameraConfiguration *param_2,int param_3,bool *param_4)

{
  undefined4 *puVar1;
  int iVar2;
  long lVar3;
  undefined8 uVar4;
  undefined8 uVar5;
  undefined8 uVar6;
  undefined8 uVar7;
  undefined8 uVar8;
  undefined8 uVar9;
  undefined *puVar10;
  VirtualCameraDevice *this;
  long lVar11;
  uint uVar12;
  undefined4 uVar13;
  undefined4 uVar14;
  ulong uVar15;
  long lVar16;
  ulong uVar17;
  undefined8 uVar18;
  undefined1 *in_x4;
  long *in_x8;
  undefined4 *puVar19;
  undefined8 uVar20;
  undefined8 uVar21;
  undefined8 *puVar22;
  char *pcVar23;
  undefined **ppuVar24;
  VirtualCameraConfiguration *pVVar25;
  undefined4 local_f4;
  VirtualCameraDevice *local_f0;
  undefined8 uStack_e8;
  char *local_e0;
  long local_d0;
  long *local_c8;
  undefined4 *local_c0;
  undefined8 uStack_b8;
  char *local_b0;
  undefined8 uStack_a8;
  undefined8 uStack_a0;
  undefined8 uStack_98;
  VirtualCameraConfiguration *local_80;
  undefined8 uStack_78;
  long local_70;
  
  uVar17 = (ulong)(uint)param_3;
  lVar3 = tpidr_el0;
  local_70 = *(long *)(lVar3 + 0x28);
  puVar22 = *(undefined8 **)(param_1 + 0x68);
  local_b0 = operator_new(0x30);
  uVar7 = s_android_permission_CREATE_VIRTUA_0010df58._32_8_;
  uVar18 = _UNK_001091b8;
  puVar19 = _DAT_001091b0;
  local_b0[0x28] = '\0';
  uStack_b8 = uVar18;
  local_c0 = puVar19;
  *(undefined8 *)(local_b0 + 0x20) = uVar7;
  uVar6 = s_android_permission_CREATE_VIRTUA_0010df58._24_8_;
  uVar5 = s_android_permission_CREATE_VIRTUA_0010df58._16_8_;
  uVar21 = s_android_permission_CREATE_VIRTUA_0010df58._8_8_;
  uVar20 = s_android_permission_CREATE_VIRTUA_0010df58._0_8_;
  *(undefined8 *)(local_b0 + 8) = s_android_permission_CREATE_VIRTUA_0010df58._8_8_;
  *(undefined8 *)local_b0 = uVar20;
  *(undefined8 *)(local_b0 + 0x18) = uVar6;
  *(undefined8 *)(local_b0 + 0x10) = uVar5;
  ppuVar24 = (undefined **)*puVar22;
  if (ppuVar24 != &PTR____node_00175c20) {
    __cfi_slowpath(0x293295f4e1fa4e55,ppuVar24);
  }
  uVar15 = (*(code *)ppuVar24[2])(puVar22,&local_c0);
  if ((uVar15 & 1) == 0) {
    puVar22 = *(undefined8 **)(param_1 + 0x68);
    local_e0 = operator_new(0x30);
    uStack_e8 = _UNK_00109318;
    local_f0 = _DAT_00109310;
    local_e0[0x2a] = '\0';
    uVar9 = s_horizonos_permission_CREATE_VIRT_0010df81._16_8_;
    uVar8 = s_horizonos_permission_CREATE_VIRT_0010df81._0_8_;
    uVar4 = CONCAT62(s_horizonos_permission_CREATE_VIRT_0010df81._26_6_,
                     s_horizonos_permission_CREATE_VIRT_0010df81._24_2_);
    *(undefined8 *)(local_e0 + 8) = s_horizonos_permission_CREATE_VIRT_0010df81._8_8_;
    *(undefined8 *)local_e0 = uVar8;
    *(undefined8 *)(local_e0 + 0x18) = uVar4;
    *(undefined8 *)(local_e0 + 0x10) = uVar9;
    uVar4 = CONCAT26(s_horizonos_permission_CREATE_VIRT_0010df81._32_2_,
                     s_horizonos_permission_CREATE_VIRT_0010df81._26_6_);
    *(undefined8 *)(local_e0 + 0x22) = s_horizonos_permission_CREATE_VIRT_0010df81._34_8_;
    *(undefined8 *)(local_e0 + 0x1a) = uVar4;
    ppuVar24 = (undefined **)*puVar22;
    if (ppuVar24 != &PTR____node_00175c20) {
      __cfi_slowpath(0x293295f4e1fa4e55,ppuVar24);
    }
    uVar12 = (*(code *)ppuVar24[2])(puVar22,&local_f0);
    uVar12 = uVar12 ^ 1;
    if (((ulong)local_f0 & 1) != 0) {
      operator_delete(local_e0);
      goto joined_r0x001326bc;
    }
    if (((ulong)local_c0 & 1) == 0) goto LAB_00132624;
LAB_0013249c:
    operator_delete(local_b0);
    if ((uVar12 & 1) == 0) goto LAB_001324a8;
LAB_00132628:
    android::IPCThreadState::self();
    uVar13 = android::IPCThreadState::getCallingPid();
    android::IPCThreadState::self();
    uVar14 = android::IPCThreadState::getCallingUid();
    __android_log_print(6,"VirtualCameraService",
                        "%s: caller (pid %d, uid %d) doesn\'t hold one of these permissions: %s, %s"
                        ,"registerCamera",uVar13,uVar14,"android.permission.CREATE_VIRTUAL_DEVICE",
                        "horizonos.permission.CREATE_VIRTUAL_CAMERA");
LAB_00132674:
    lVar11 = AStatus_fromExceptionCode(0xffffffff);
  }
  else {
    uVar12 = 0;
joined_r0x001326bc:
    if (((ulong)local_c0 & 1) != 0) goto LAB_0013249c;
LAB_00132624:
    if ((uVar12 & 1) != 0) goto LAB_00132628;
LAB_001324a8:
    if (*(uint *)(uVar17 + 0x80) < 2) {
      puVar22 = *(undefined8 **)(param_1 + 0x68);
      local_b0 = operator_new(0x30);
      *(undefined8 *)(local_b0 + 0x20) = uVar7;
      local_b0[0x28] = '\0';
      uStack_b8 = uVar18;
      local_c0 = puVar19;
      *(undefined8 *)(local_b0 + 8) = uVar21;
      *(undefined8 *)local_b0 = uVar20;
      *(undefined8 *)(local_b0 + 0x18) = uVar6;
      *(undefined8 *)(local_b0 + 0x10) = uVar5;
      ppuVar24 = (undefined **)*puVar22;
      if (ppuVar24 != &PTR____node_00175c20) {
        __cfi_slowpath(0x293295f4e1fa4e55,ppuVar24);
      }
      uVar15 = (*(code *)ppuVar24[2])(puVar22,&local_c0);
      if (((ulong)local_c0 & 1) != 0) {
        operator_delete(local_b0);
      }
      if ((uVar15 & 1) == 0) {
        android::IPCThreadState::self();
        uVar13 = android::IPCThreadState::getCallingPid();
        android::IPCThreadState::self();
        uVar14 = android::IPCThreadState::getCallingUid();
        __android_log_print(6,"VirtualCameraService",
                            "%s: caller (pid %d, uid %d) doesn\'t hold %s permission which is necessary to create cameraSource PASSTHROUGH or AVATAR. Consider using cameraSource APP_DEFINED"
                            ,"registerCamera",uVar13,uVar14,
                            "android.permission.CREATE_VIRTUAL_DEVICE");
        goto LAB_00132674;
      }
    }
    if (in_x4 != (undefined1 *)0x0) {
      if (param_1[0x60] != (SpAIBinder)0x0) {
        *in_x8 = 0;
        local_d0 = 0;
        local_c8 = (long *)0x0;
        uStack_b8 = 0;
        local_c0 = (undefined4 *)0x0;
        uStack_a8 = 0;
        local_b0 = (char *)0x0;
        uStack_98 = 0;
        uStack_a0 = 0;
        EglDisplayContext::EglDisplayContext((EglDisplayContext *)&local_c0,&local_d0);
        if (local_c8 != (long *)0x0) {
          LOAcquire();
          lVar11 = local_c8[1];
          local_c8[1] = lVar11 + -1;
          LORelease();
          if (lVar11 == 0) {
            (**(code **)(*local_c8 + 0x10))(local_c8);
            std::__shared_weak_count::__release_weak();
          }
        }
        pcVar23 = "GL_OES_EGL_image_external";
        uVar15 = isGlExtensionSupported("GL_OES_EGL_image_external");
        if ((uVar15 & 1) == 0) {
LAB_001326d0:
          __android_log_print(6,"VirtualCameraService","%s not supported",pcVar23);
          uStack_78 = 0;
          local_80 = (VirtualCameraConfiguration *)pcVar23;
          fmt::v7::detail::vformat
                    (&local_f0,
                     "Cannot create virtual camera, because required EGL extension {} is not supported on this system"
                     ,0x5f,0xc,&local_80);
          pcVar23 = (char *)((ulong)&local_f0 | 1);
          if (((ulong)local_f0 & 1) != 0) {
            pcVar23 = local_e0;
          }
          lVar11 = AStatus_fromExceptionCodeWithMessage(0xfffffff9,pcVar23);
          *in_x8 = lVar11;
          if (((ulong)local_f0 & 1) != 0) {
            operator_delete(local_e0);
          }
        }
        else {
          pcVar23 = "GL_OES_EGL_image_external_essl3";
          uVar15 = isGlExtensionSupported("GL_OES_EGL_image_external_essl3");
          if ((uVar15 & 1) == 0) goto LAB_001326d0;
          pcVar23 = "GL_EXT_YUV_target";
          uVar15 = isGlExtensionSupported("GL_EXT_YUV_target");
          if ((uVar15 & 1) == 0) goto LAB_001326d0;
          lVar11 = AStatus_newOk();
          *in_x8 = lVar11;
        }
        EglDisplayContext::~EglDisplayContext((EglDisplayContext *)&local_c0);
        if ((lVar11 == 0) || (uVar15 = AStatus_isOk(lVar11), (uVar15 & 1) == 0)) {
          *in_x4 = 0;
          goto LAB_00132680;
        }
        __cfi_slowpath(0xad4b6bf053f7ac1b,PTR_AStatus_delete_00176f28);
        AStatus_delete(lVar11);
      }
      local_d0 = 0;
      if (*(undefined ***)param_1 != &PTR__VirtualCameraService_00176640) {
        __cfi_slowpath(0x7fd7667d391a1952);
      }
      validateCameraId((VirtualCameraConfiguration *)param_1);
      lVar11 = local_d0;
      if ((local_d0 == 0) || (uVar15 = AStatus_isOk(local_d0), (uVar15 & 1) == 0)) {
        lVar16 = 0;
        *in_x4 = 0;
        *in_x8 = lVar11;
      }
      else {
        puVar19 = *(undefined4 **)(uVar17 + 0x18);
        puVar1 = *(undefined4 **)(uVar17 + 0x20);
        if (puVar19 == puVar1) {
          pcVar23 = "%s: No supported input configuration specified";
LAB_0013289c:
          __android_log_print(6,"VirtualCameraService",pcVar23,"validateConfiguration");
LAB_001328d8:
          lVar16 = AStatus_fromServiceSpecificError(0xfffffffd);
        }
        else {
          if (*(long *)(uVar17 + 0x30) == 0) {
            pcVar23 = "%s: Input configuration is missing virtual camera callback";
            goto LAB_0013289c;
          }
          do {
            uVar15 = isFormatSupportedForInput(*puVar19,puVar19[1],puVar19[2],puVar19[3]);
            if ((uVar15 & 1) == 0) {
              __android_log_print(6,"VirtualCameraService",
                                  "%s: Requested unsupported input format: %d x %d (%d)",
                                  "validateConfiguration",*puVar19,puVar19[1],puVar19[2]);
              goto LAB_001328d8;
            }
            puVar19 = puVar19 + 4;
          } while (puVar19 != puVar1);
          iVar2 = *(int *)(uVar17 + 0x40);
          if (0xb3 < iVar2) {
            if ((iVar2 == 0xb4) || (iVar2 == 0x10e)) goto LAB_00132a28;
            goto LAB_001328d8;
          }
          if ((iVar2 != 0) && (iVar2 != 0x5a)) goto LAB_001328d8;
LAB_00132a28:
          if (*(int *)(uVar17 + 0x80) == 2) {
            if (*(uint *)(uVar17 + 0x44) != 2) {
              __android_log_print(6,"VirtualCameraService",
                                  "%s: Invalid camera LensFacing %d, APP_DEFINED cameras must set LensFacing to External."
                                  ,"validateConfiguration");
              goto LAB_001328d8;
            }
          }
          else if (2 < *(uint *)(uVar17 + 0x44)) goto LAB_001328d8;
          lVar16 = AStatus_newOk();
        }
        puVar10 = PTR_AStatus_delete_00176f28;
        __cfi_slowpath(0xad4b6bf053f7ac1b,PTR_AStatus_delete_00176f28);
        AStatus_delete(lVar11);
        __cfi_slowpath(0xad4b6bf053f7ac1b,puVar10);
        AStatus_delete(0);
        if ((lVar16 == 0) || (uVar17 = AStatus_isOk(lVar16), (uVar17 & 1) == 0)) {
          *in_x4 = 0;
          *in_x8 = lVar16;
          lVar16 = 0;
        }
        else {
          std::mutex::lock();
          android::IPCThreadState::self();
          local_f4 = android::IPCThreadState::getCallingUid();
          local_c0 = &local_f4;
          lVar11 = std::
                   __hash_table<std::__hash_value_type<unsigned_int,std::unordered_set<ndk::SpAIBinder,android::companion::virtualcamera::VirtualCameraService::BinderTokenHash,std::equal_to<ndk::SpAIBinder>,std::allocator<ndk::SpAIBinder>>>,std::__unordered_map_hasher<unsigned_int,std::__hash_value_type<unsigned_int,std::unordered_set<ndk::SpAIBinder,android::companion::virtualcamera::VirtualCameraService::BinderTokenHash,std::equal_to<ndk::SpAIBinder>,std::allocator<ndk::SpAIBinder>>>,std::hash<unsigned_int>,true>,std::__unordered_map_equal<unsigned_int,std::__hash_value_type<unsigned_int,std::unordered_set<ndk::SpAIBinder,android::companion::virtualcamera::VirtualCameraService::BinderTokenHash,std::equal_to<ndk::SpAIBinder>,std::allocator<ndk::SpAIBinder>>>,std::equal_to<unsigned_int>,true>,std::allocator<std::__hash_value_type<unsigned_int,std::unordered_set<ndk::SpAIBinder,android::companion::virtualcamera::VirtualCameraService::BinderTokenHash,std::equal_to<ndk::SpAIBinder>,std::allocator<ndk::SpAIBinder>>>>>
                   ::
                   __emplace_unique_key_args<unsigned_int,std::piecewise_construct_t_const&,std::tuple<unsigned_int_const&>,std::tuple<>>
                             ((uint *)(param_1 + 0xc0),(piecewise_construct_t *)&local_f4,
                              (tuple *)&DAT_0010e21b,(tuple *)&local_c0);
          if (*(ulong *)(lVar11 + 0x30) < (ulong)(long)DAT_00178bc0) {
            lVar11 = std::
                     unordered_map<ndk::SpAIBinder,std::string,android::companion::virtualcamera::VirtualCameraService::BinderTokenHash,std::equal_to<ndk::SpAIBinder>,std::allocator<std::pair<ndk::SpAIBinder_const,std::string>>>
                     ::find((unordered_map<ndk::SpAIBinder,std::string,android::companion::virtualcamera::VirtualCameraService::BinderTokenHash,std::equal_to<ndk::SpAIBinder>,std::allocator<std::pair<ndk::SpAIBinder_const,std::string>>>
                             *)(param_1 + 0x98),(SpAIBinder *)param_2);
            if (lVar11 == 0) {
              pVVar25 = *(VirtualCameraConfiguration **)(param_1 + 0x50);
              local_f0 = (VirtualCameraDevice *)0x0;
              uStack_e8 = 0;
              if (*(undefined ***)pVVar25 != &PTR__VirtualCameraProvider_00176040) {
                __cfi_slowpath(0x56f3385870b5b2d3);
              }
              VirtualCameraProvider::createCamera(pVVar25,param_3);
              this = local_f0;
              if (local_f0 == (VirtualCameraDevice *)0x0) {
                __android_log_print(6,"VirtualCameraService",
                                    "Failed to create camera for binder token 0x%lx",
                                    *(undefined8 *)param_2);
                *in_x4 = 0;
                lVar11 = AStatus_fromServiceSpecificError(0xfffffff8);
              }
              else {
                if (*(undefined ***)local_f0 != &PTR__VirtualCameraDevice_00176220) {
                  __cfi_slowpath(0x8d0ad1676903e960);
                }
                VirtualCameraDevice::getCameraName(this);
                local_80 = param_2;
                lVar11 = std::
                         __hash_table<std::__hash_value_type<ndk::SpAIBinder,std::string>,std::__unordered_map_hasher<ndk::SpAIBinder,std::__hash_value_type<ndk::SpAIBinder,std::string>,android::companion::virtualcamera::VirtualCameraService::BinderTokenHash,true>,std::__unordered_map_equal<ndk::SpAIBinder,std::__hash_value_type<ndk::SpAIBinder,std::string>,std::equal_to<ndk::SpAIBinder>,true>,std::allocator<std::__hash_value_type<ndk::SpAIBinder,std::string>>>
                         ::
                         __emplace_unique_key_args<ndk::SpAIBinder,std::piecewise_construct_t_const&,std::tuple<ndk::SpAIBinder_const&>,std::tuple<>>
                                   (param_1 + 0x98,(piecewise_construct_t *)param_2,
                                    (tuple *)&DAT_0010e21b,(tuple *)&local_80);
                std::string::__move_assign(lVar11 + 0x18,&local_c0);
                if (((ulong)local_c0 & 1) != 0) {
                  operator_delete(local_b0);
                }
                local_c0 = &local_f4;
                lVar11 = std::
                         __hash_table<std::__hash_value_type<unsigned_int,std::unordered_set<ndk::SpAIBinder,android::companion::virtualcamera::VirtualCameraService::BinderTokenHash,std::equal_to<ndk::SpAIBinder>,std::allocator<ndk::SpAIBinder>>>,std::__unordered_map_hasher<unsigned_int,std::__hash_value_type<unsigned_int,std::unordered_set<ndk::SpAIBinder,android::companion::virtualcamera::VirtualCameraService::BinderTokenHash,std::equal_to<ndk::SpAIBinder>,std::allocator<ndk::SpAIBinder>>>,std::hash<unsigned_int>,true>,std::__unordered_map_equal<unsigned_int,std::__hash_value_type<unsigned_int,std::unordered_set<ndk::SpAIBinder,android::companion::virtualcamera::VirtualCameraService::BinderTokenHash,std::equal_to<ndk::SpAIBinder>,std::allocator<ndk::SpAIBinder>>>,std::equal_to<unsigned_int>,true>,std::allocator<std::__hash_value_type<unsigned_int,std::unordered_set<ndk::SpAIBinder,android::companion::virtualcamera::VirtualCameraService::BinderTokenHash,std::equal_to<ndk::SpAIBinder>,std::allocator<ndk::SpAIBinder>>>>>
                         ::
                         __emplace_unique_key_args<unsigned_int,std::piecewise_construct_t_const&,std::tuple<unsigned_int_const&>,std::tuple<>>
                                   ((uint *)(param_1 + 0xc0),(piecewise_construct_t *)&local_f4,
                                    (tuple *)&DAT_0010e21b,(tuple *)&local_c0);
                std::
                __hash_table<ndk::SpAIBinder,android::companion::virtualcamera::VirtualCameraService::BinderTokenHash,std::equal_to<ndk::SpAIBinder>,std::allocator<ndk::SpAIBinder>>
                ::__emplace_unique_key_args<ndk::SpAIBinder,ndk::SpAIBinder_const&>
                          ((__hash_table<ndk::SpAIBinder,android::companion::virtualcamera::VirtualCameraService::BinderTokenHash,std::equal_to<ndk::SpAIBinder>,std::allocator<ndk::SpAIBinder>>
                            *)(lVar11 + 0x18),(SpAIBinder *)param_2,(SpAIBinder *)param_2);
                uVar20 = *(undefined8 *)param_2;
                uVar21 = *(undefined8 *)(param_1 + 0xf0);
                puVar22 = operator_new(0x10);
                uVar18 = AIBinder_Weak_new(uVar20);
                *puVar22 = param_1;
                puVar22[1] = uVar18;
                local_c0 = (undefined4 *)0x0;
                AIBinder_linkToDeath(uVar20,uVar21,puVar22);
                ndk::impl::ScopedAResource<AIBinder_Weak*,&AIBinder_Weak_delete,(AIBinder_Weak*)0>::
                set((ScopedAResource<AIBinder_Weak*,&AIBinder_Weak_delete,(AIBinder_Weak*)0> *)
                    &local_c0,(AIBinder_Weak *)0x0);
                *in_x4 = 1;
                lVar11 = AStatus_newOk();
              }
              *in_x8 = lVar11;
              std::shared_ptr<android::companion::virtualcamera::VirtualCameraDevice>::~shared_ptr
                        ((shared_ptr<android::companion::virtualcamera::VirtualCameraDevice> *)
                         &local_f0);
            }
            else {
              __android_log_print(6,"VirtualCameraService",
                                  "Attempt to register camera corresponding to already registered binder token: 0x%lx"
                                  ,*(undefined8 *)param_2);
              *in_x4 = 0;
              lVar11 = AStatus_newOk();
              *in_x8 = lVar11;
            }
          }
          else {
            __android_log_print(6,"VirtualCameraService",
                                "Client UID %d has reached the maximum number of registered cameras (%d)"
                                ,local_f4);
            *in_x4 = 0;
            lVar11 = AStatus_fromServiceSpecificError(0xfffffffd);
            *in_x8 = lVar11;
          }
          std::mutex::unlock();
        }
      }
      __cfi_slowpath(0xad4b6bf053f7ac1b,PTR_AStatus_delete_00176f28);
      AStatus_delete(lVar16);
      goto LAB_00132680;
    }
    lVar11 = AStatus_fromServiceSpecificError(0xfffffffd);
  }
  *in_x8 = lVar11;
LAB_00132680:
  if (*(long *)(lVar3 + 0x28) == local_70) {
    return;
  }
                    /* WARNING: Subroutine does not return */
  __stack_chk_fail();
}



====================================================================================================
// getSessionLogId @ 00133450

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */
/* android::companion::virtualcamera::VirtualCameraService::getSessionLogId(ndk::SpAIBinder const&,
   long*) */

void android::companion::virtualcamera::VirtualCameraService::getSessionLogId
               (SpAIBinder *param_1,long *param_2)

{
  long lVar1;
  undefined8 uVar2;
  undefined8 uVar3;
  undefined8 uVar4;
  long *plVar5;
  long lVar6;
  uint uVar7;
  undefined4 uVar8;
  undefined4 uVar9;
  ulong uVar10;
  undefined8 uVar11;
  long *in_x2;
  undefined8 *in_x8;
  undefined **ppuVar12;
  undefined8 *puVar13;
  ulong local_90;
  undefined8 uStack_88;
  char *local_80;
  long *local_70;
  long *plStack_68;
  char *local_60;
  long local_58;
  
  lVar1 = tpidr_el0;
  local_58 = *(long *)(lVar1 + 0x28);
  puVar13 = *(undefined8 **)(param_1 + 0x68);
  local_60 = operator_new(0x30);
  plStack_68 = _UNK_001091b8;
  local_70 = _DAT_001091b0;
  local_60[0x28] = '\0';
  uVar4 = s_android_permission_CREATE_VIRTUA_0010df58._24_8_;
  uVar3 = s_android_permission_CREATE_VIRTUA_0010df58._16_8_;
  uVar2 = s_android_permission_CREATE_VIRTUA_0010df58._8_8_;
  uVar11 = s_android_permission_CREATE_VIRTUA_0010df58._0_8_;
  *(undefined8 *)(local_60 + 0x20) = s_android_permission_CREATE_VIRTUA_0010df58._32_8_;
  *(undefined8 *)(local_60 + 8) = uVar2;
  *(undefined8 *)local_60 = uVar11;
  *(undefined8 *)(local_60 + 0x18) = uVar4;
  *(undefined8 *)(local_60 + 0x10) = uVar3;
  ppuVar12 = (undefined **)*puVar13;
  if (ppuVar12 != &PTR____node_00175c20) {
    __cfi_slowpath(0x293295f4e1fa4e55,ppuVar12);
  }
  uVar10 = (*(code *)ppuVar12[2])(puVar13,&local_70);
  if ((uVar10 & 1) == 0) {
    puVar13 = *(undefined8 **)(param_1 + 0x68);
    local_80 = operator_new(0x30);
    uStack_88 = _UNK_00109318;
    local_90 = _DAT_00109310;
    local_80[0x2a] = '\0';
    uVar3 = s_horizonos_permission_CREATE_VIRT_0010df81._16_8_;
    uVar2 = s_horizonos_permission_CREATE_VIRT_0010df81._0_8_;
    uVar11 = CONCAT62(s_horizonos_permission_CREATE_VIRT_0010df81._26_6_,
                      s_horizonos_permission_CREATE_VIRT_0010df81._24_2_);
    *(undefined8 *)(local_80 + 8) = s_horizonos_permission_CREATE_VIRT_0010df81._8_8_;
    *(undefined8 *)local_80 = uVar2;
    *(undefined8 *)(local_80 + 0x18) = uVar11;
    *(undefined8 *)(local_80 + 0x10) = uVar3;
    uVar11 = CONCAT26(s_horizonos_permission_CREATE_VIRT_0010df81._32_2_,
                      s_horizonos_permission_CREATE_VIRT_0010df81._26_6_);
    *(undefined8 *)(local_80 + 0x22) = s_horizonos_permission_CREATE_VIRT_0010df81._34_8_;
    *(undefined8 *)(local_80 + 0x1a) = uVar11;
    ppuVar12 = (undefined **)*puVar13;
    if (ppuVar12 != &PTR____node_00175c20) {
      __cfi_slowpath(0x293295f4e1fa4e55,ppuVar12);
    }
    uVar7 = (*(code *)ppuVar12[2])(puVar13,&local_90);
    uVar7 = uVar7 ^ 1;
    if ((local_90 & 1) != 0) {
      operator_delete(local_80);
    }
  }
  else {
    uVar7 = 0;
  }
  if (((ulong)local_70 & 1) != 0) {
    operator_delete(local_60);
  }
  if ((uVar7 & 1) == 0) {
    if (in_x2 == (long *)0x0) {
      uVar11 = AStatus_fromServiceSpecificError(0xfffffffd);
      *in_x8 = uVar11;
    }
    else {
      local_70 = (long *)0x0;
      plStack_68 = (long *)0x0;
      if (*(undefined ***)param_1 != &PTR__VirtualCameraService_00176640) {
        __cfi_slowpath(0x7fd7667d391a1952);
      }
      getCamera(param_1);
      plVar5 = local_70;
      if (local_70 == (long *)0x0) {
        __android_log_print(6,"VirtualCameraService",
                            "Attempt to get session log id corresponding to unknown binder token: 0x%lx"
                            ,*param_2);
      }
      else {
        if ((undefined **)*local_70 != &PTR__VirtualCameraDevice_00176220) {
          __cfi_slowpath(0x8d0ad1676903e960);
        }
        *in_x2 = plVar5[0x18];
      }
      uVar11 = AStatus_newOk();
      *in_x8 = uVar11;
      if (plStack_68 != (long *)0x0) {
        LOAcquire();
        lVar6 = plStack_68[1];
        plStack_68[1] = lVar6 + -1;
        LORelease();
        if (lVar6 == 0) {
          (**(code **)(*plStack_68 + 0x10))(plStack_68);
          std::__shared_weak_count::__release_weak();
        }
      }
    }
  }
  else {
    android::IPCThreadState::self();
    uVar8 = android::IPCThreadState::getCallingPid();
    android::IPCThreadState::self();
    uVar9 = android::IPCThreadState::getCallingUid();
    __android_log_print(6,"VirtualCameraService",
                        "%s: caller (pid %d, uid %d) doesn\'t hold one of these permissions: %s, %s"
                        ,"getSessionLogId",uVar8,uVar9,"android.permission.CREATE_VIRTUAL_DEVICE",
                        "horizonos.permission.CREATE_VIRTUAL_CAMERA");
    uVar11 = AStatus_fromExceptionCode(0xffffffff);
    *in_x8 = uVar11;
  }
  if (*(long *)(lVar1 + 0x28) != local_58) {
                    /* WARNING: Subroutine does not return */
    __stack_chk_fail();
  }
  return;
}



====================================================================================================
// validateCameraId @ 00133c60

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */
/* android::companion::virtualcamera::VirtualCameraService::validateCameraId(aidl::android::companion::virtualcamera::VirtualCameraConfiguration
   const&) */

void android::companion::virtualcamera::VirtualCameraService::validateCameraId
               (VirtualCameraConfiguration *param_1)

{
  byte *pbVar1;
  long lVar2;
  undefined8 uVar3;
  undefined8 uVar4;
  undefined8 uVar5;
  char cVar6;
  bool bVar7;
  undefined4 uVar8;
  undefined4 uVar9;
  int iVar10;
  ulong uVar11;
  undefined8 uVar12;
  byte *in_x1;
  undefined8 *in_x8;
  undefined **ppuVar13;
  undefined8 *puVar14;
  void *local_140;
  void *pvStack_138;
  undefined8 local_130;
  undefined8 uStack_128;
  undefined8 uStack_120;
  undefined8 uStack_118;
  undefined8 local_110;
  undefined8 uStack_108;
  undefined8 uStack_100;
  undefined8 uStack_f8;
  undefined8 local_f0;
  undefined8 uStack_e8;
  undefined8 uStack_e0;
  undefined8 uStack_d8;
  void *local_d0;
  void *pvStack_c8;
  char *local_c0;
  undefined8 uStack_b8;
  undefined8 uStack_b0;
  undefined8 uStack_a8;
  undefined8 local_a0;
  undefined8 uStack_98;
  undefined8 uStack_90;
  undefined8 uStack_88;
  undefined8 local_80;
  undefined8 uStack_78;
  undefined8 uStack_70;
  undefined8 uStack_68;
  long local_58;
  
  lVar2 = tpidr_el0;
  local_58 = *(long *)(lVar2 + 0x28);
  if (((DAT_00178c08 & 1) == 0) && (iVar10 = __cxa_guard_acquire(&DAT_00178c08), iVar10 != 0)) {
    std::regex::basic_regex((regex *)&DAT_00178bc8,"[a-zA-Z0-9]+",0);
    __cxa_atexit(std::regex::~basic_regex,&DAT_00178bc8,&DAT_00178bb8);
    __cxa_guard_release(&DAT_00178c08);
  }
  uVar11 = (ulong)(*in_x1 >> 1);
  if ((*in_x1 & 1) != 0) {
    uVar11 = *(ulong *)(in_x1 + 8);
  }
  if (uVar11 != 0) {
    puVar14 = *(undefined8 **)(param_1 + 0x68);
    local_c0 = operator_new(0x30);
    pvStack_c8 = _UNK_001091b8;
    local_d0 = _DAT_001091b0;
    local_c0[0x28] = '\0';
    uVar5 = s_android_permission_CREATE_VIRTUA_0010df58._24_8_;
    uVar4 = s_android_permission_CREATE_VIRTUA_0010df58._16_8_;
    uVar3 = s_android_permission_CREATE_VIRTUA_0010df58._8_8_;
    uVar12 = s_android_permission_CREATE_VIRTUA_0010df58._0_8_;
    *(undefined8 *)(local_c0 + 0x20) = s_android_permission_CREATE_VIRTUA_0010df58._32_8_;
    *(undefined8 *)(local_c0 + 8) = uVar3;
    *(undefined8 *)local_c0 = uVar12;
    *(undefined8 *)(local_c0 + 0x18) = uVar5;
    *(undefined8 *)(local_c0 + 0x10) = uVar4;
    ppuVar13 = (undefined **)*puVar14;
    if (ppuVar13 != &PTR____node_00175c20) {
      __cfi_slowpath(0x293295f4e1fa4e55,ppuVar13);
    }
    uVar11 = (*(code *)ppuVar13[2])(puVar14,&local_d0);
    if (((ulong)local_d0 & 1) != 0) {
      operator_delete(local_c0);
    }
    if ((uVar11 & 1) == 0) {
      android::IPCThreadState::self();
      uVar8 = android::IPCThreadState::getCallingPid();
      android::IPCThreadState::self();
      uVar9 = android::IPCThreadState::getCallingUid();
      __android_log_print(6,"VirtualCameraService",
                          "%s: Camera producer is not allowed to set cameraId. Caller (pid %d, uid %d) doesn\'t hold permission %s"
                          ,"validateCameraId",uVar8,uVar9,"android.permission.CREATE_VIRTUAL_DEVICE"
                         );
      uVar12 = AStatus_fromExceptionCode(0xffffffff);
      goto LAB_00133e6c;
    }
    uVar11 = *(ulong *)(in_x1 + 8);
    pbVar1 = *(byte **)(in_x1 + 0x10);
    if ((*in_x1 & 1) == 0) {
      pbVar1 = in_x1 + 1;
      uVar11 = (ulong)(*in_x1 >> 1);
    }
    uStack_128 = 0;
    local_130 = 0;
    uStack_118 = 0;
    uStack_120 = 0;
    uStack_108 = 0;
    local_110 = 0;
    uStack_f8 = 0;
    uStack_100 = 0;
    uStack_e8 = 0;
    local_f0 = 0;
    uStack_d8 = 0;
    uStack_e0 = 0;
    pvStack_138 = (void *)0x0;
    local_140 = (void *)0x0;
    pvStack_c8 = (void *)0x0;
    local_d0 = (void *)0x0;
    uStack_b8 = 0;
    local_c0 = (char *)0x0;
    uStack_a8 = 0;
    uStack_b0 = 0;
    uStack_98 = 0;
    local_a0 = 0;
    uStack_88 = 0;
    uStack_90 = 0;
    uStack_78 = 0;
    local_80 = 0;
    uStack_68 = 0;
    uStack_70 = 0;
    bVar7 = std::regex::__search<std::allocator<std::sub_match<char_const*>>>
                      ((regex *)&DAT_00178bc8,pbVar1,pbVar1 + uVar11,&local_d0,0x1040);
    std::
    match_results<std::__wrap_iter<char_const*>,std::allocator<std::sub_match<std::__wrap_iter<char_const*>>>>
    ::__assign<char_const*,std::allocator<std::sub_match<char_const*>>>
              ((match_results<std::__wrap_iter<char_const*>,std::allocator<std::sub_match<std::__wrap_iter<char_const*>>>>
                *)&local_140,pbVar1,pbVar1 + uVar11,&local_d0,0);
    if (local_d0 != (void *)0x0) {
      pvStack_c8 = local_d0;
      operator_delete(local_d0);
    }
    cVar6 = (char)uStack_e8;
    if (local_140 != (void *)0x0) {
      pvStack_138 = local_140;
      operator_delete(local_140);
    }
    if (!bVar7 || cVar6 != '\0') {
      pbVar1 = in_x1 + 1;
      if ((*in_x1 & 1) != 0) {
        pbVar1 = *(byte **)(in_x1 + 0x10);
      }
      __android_log_print(6,"VirtualCameraService",
                          "%s: Invalid camera id, has to consist entirely of alphanumeric characters (with at least one character): %s"
                          ,"validateCameraId",pbVar1);
      uVar12 = AStatus_fromServiceSpecificError(0xfffffffd);
      goto LAB_00133e6c;
    }
  }
  uVar12 = AStatus_newOk();
LAB_00133e6c:
  *in_x8 = uVar12;
  if (*(long *)(lVar2 + 0x28) == local_58) {
    return;
  }
                    /* WARNING: Subroutine does not return */
  __stack_chk_fail();
}



