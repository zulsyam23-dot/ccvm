# Spesifikasi IR (Intermediate Representation) CCVM v1.0

## 1. Pendahuluan

CCVM IR merupakan representasi intermediate yang dirancang untuk optimalitas kompilasi dan kemudahan optimasi. IR ini berbasis SSA (Static Single Assignment) dengan ekstensi untuk paralelisme dan vectorization.

## 2. Format IR

### 2.1 Format Teks (Human-Readable)
```
; Module: example.ccvm
; Target: x86_64-linux-gnu
; ABI: sysv

module example {
    ; Function definition
    func @main() -> i32 {
        entry:
            %0 = constant i32 0
            %1 = call @printf(ptr @hello_world, %0)
            ret i32 %0
    }
    
    ; Global data
    @hello_world = global [14 x i8] c"Hello, World!\0A\00"
}
```

### 2.2 Format Biner (Compact)
- Magic header: `CCVM\x01\x00\x00\x00`
- Endianness: Little-endian
- Alignment: 8-byte boundary

## 3. Type System

### 3.1 Primitive Types
```
i1   : 1-bit integer (boolean)
i8   : 8-bit integer
i16  : 16-bit integer
i32  : 32-bit integer
i64  : 64-bit integer
i128 : 128-bit integer
f16  : 16-bit floating point (half-precision)
f32  : 32-bit floating point (single-precision)
f64  : 64-bit floating point (double-precision)
f128 : 128-bit floating point (quad-precision)
```

### 3.2 Composite Types
```
[T x N] : Array dengan T elemen bertipe N
{T1, T2, ...} : Tuple dengan tipe T1, T2, ...
struct {field1: T1, field2: T2} : Struct dengan field bertipe T1, T2
ptr T : Pointer ke tipe T
func(T1, T2, ...) -> R : Function type
```

### 3.3 Vector Types
```
vec<T x N> : Vector dengan N elemen bertipe T
```

## 4. Instruction Set

### 4.1 Memory Instructions
```
alloca T, align N           ; Allocate stack memory
load T, ptr P, align N     ; Load dari memory
store T V, ptr P, align N    ; Store ke memory
getelementptr T, ptr P, i32 I ; Calculate address
```

### 4.2 Arithmetic Instructions
```
add T, V1, V2    ; Addition
sub T, V1, V2    ; Subtraction
mul T, V1, V2    ; Multiplication
udiv T, V1, V2   ; Unsigned division
sdiv T, V1, V2   ; Signed division
urem T, V1, V2   ; Unsigned remainder
srem T, V1, V2   ; Signed remainder
```

### 4.3 Logical Instructions
```
and T, V1, V2     ; Bitwise AND
or T, V1, V2      ; Bitwise OR
xor T, V1, V2     ; Bitwise XOR
shl T, V1, V2     ; Shift left
lshr T, V1, V2    ; Logical shift right
ashr T, V1, V2    ; Arithmetic shift right
```

### 4.4 Comparison Instructions
```
icmp pred, T, V1, V2  ; Integer comparison
fcmp pred, T, V1, V2  ; Floating comparison
```

### 4.5 Control Flow Instructions
```
br label L                    ; Unconditional branch
br i1 C, label T, label F     ; Conditional branch
switch T V, label D [T1, L1, ...]
ret T V                       ; Return value
ret void                      ; Return void
```

### 4.6 Function Instructions
```
call T @func(T1 V1, ...)      ; Function call
invoke T @func(T1 V1, ...) to label N unwind label E
```

### 4.7 Vector Instructions
```
extractelement vec<T x N> V, i32 I
insertelement vec<T x N> V, T E, i32 I
shufflevector vec<T x N> V1, vec<T x N> V2, vec<M x i32> M
```

## 5. Calling Convention

### 5.1 Parameter Passing
- System V AMD64 ABI untuk x86_64
- AAPCS64 untuk ARM64
- RISC-V Calling Convention untuk RISC-V

### 5.2 Register Allocation
- Caller-saved: RAX, RCX, RDX, RSI, RDI, R8-R11
- Callee-saved: RBX, RBP, R12-R15
- Return value: RAX (integer), XMM0 (floating)

## 6. Metadata dan Debug Info

### 6.1 Source Location
```
!dbg !{i32 line, i32 column, metadata scope, metadata inlinedAt}
```

### 6.2 Type Information
```
!tbaa !{metadata type, metadata parent}
```

### 6.3 Optimization Hints
```
!alias.scope !{metadata scope}
!noalias !{metadata scope}
```

## 7. Optimisasi IR

### 7.1 Pass Optimisasi
1. **Mem2Reg**: Promosi memory ke register
2. **GVN**: Global Value Numbering
3. **LICM**: Loop Invariant Code Motion
4. **Vectorization**: Loop vectorization
5. **Inlining**: Function inlining

### 7.2 Analysis Pass
1. **Alias Analysis**: Analisis aliasing
2. **Dominance**: Dominator tree analysis
3. **Loop Analysis**: Loop detection dan analysis
4. **Call Graph**: Call graph construction

## 8. Validasi IR

### 8.1 Type Checking
- Semua instruction harus type-safe
- Pointer arithmetic harus valid
- Function signatures harus match

### 8.2 SSA Validation
- Setiap SSA value hanya defined sekali
- Phi nodes harus di basic block header
- Uses harus dominated oleh definitions

### 8.3 Memory Validation
- Load/store harus aligned
- Pointer arithmetic harus in-bound
- Memory leaks detection

## 9. Ekstensi Masa Depan

### 9.1 Parallel IR
- Thread creation dan synchronization
- Atomic operations
- Memory barriers

### 9.2 Machine Learning IR
- Tensor operations
- Automatic differentiation
- Quantization support

### 9.3 WebAssembly IR
- WebAssembly text format support
- WASI integration
- Browser optimization

## 10. Referensi

- [LLVM Language Reference](https://llvm.org/docs/LangRef.html)
- [SSA-based Compiler Design](https://ssabook.gforge.inria.fr/)
- [Modern Compiler Implementation](https://www.cs.princeton.edu/~appel/modern/)
- [Engineering a Compiler](https://www.elsevier.com/books/engineering-a-compiler/cooper/978-0-12-815579-4)