; ModuleID = '<stdin>'
target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-f80:32:32-v16:16:16-v24:32:32-v32:32:32-v48:64:64-v64:64:64-v96:128:128-v128:128:128-v192:256:256-v256:256:256-v512:512:512-v1024:1024:1024-a0:0:64"
target triple = "amdil-pc-amdopencl"

declare void @llvm.skir.pop(i32, i8*) nounwind

declare void @llvm.skir.push(i32, i8*) nounwind

declare i32 @get_global_id(i32) nounwind

declare float @sqrt(float) nounwind
declare float @sqrtf(float) nounwind
