# CCVM (Cross-Compiler Virtual Machine)

## Visi dan Misi

### Visi
CCVM menjadi infrastruktur compiler generasi baru yang menggabungkan kekuatan multi-bahasa pemrograman untuk menghasilkan native code berkualitas produksi dengan performa kompetitif dan learning curve yang rendah.

### Misi
1. Menyediakan alternatif LLVM dengan filosofi desain yang berbeda
2. Memanfaatkan kekuatan setiap bahasa pemrograman untuk komponen yang sesuai
3. Menghasilkan native code murni tanpa runtime dependency
4. Menyediakan dua lapisan: low-level deterministik dan high-level fleksibel

---

## Pencapaian Saat Ini (Update: April 2026)

### Pipeline Lengkap (End-to-End)
CCVM berhasil mengkompilasi kode C menjadi executable native Windows x86_64 melalui pipeline lengkap:

```
Source (.c) → Lexer → Parser → AST → IR → Backend (x86_64) → MASM → Linker → EXE
```

### Fitur yang Sudah Berfungsi

| Komponen | Fitur | Status |
|---|---|---|
| **Lexer** | Tokenisasi C (keyword, identifier, operator, literal) | ✅ |
| **Parser** | AST untuk function definition, variable declaration, binary expression, if/else, return | ✅ |
| **IR Generator** | SSA-like IR text format dengan temp variables, alloca, store, load, call, icmp, branch | ✅ |
| **IR Parser** | Parsing IR text ke C structs untuk backend (termasuk call args, labels, icmp) | ✅ |
| **Backend x86_64** | Instruction selection ke MASM syntax, stack-based allocation, Win64 ABI | ✅ |
| **Assembler** | Integrasi MASM (`ml64.exe`) untuk assembly → object file | ✅ |
| **Linker** | Integrasi MSVC `link.exe` + `kernel32.lib` → native .exe | ✅ |

### Fitur Bahasa C yang Dikompilasi

| Fitur | Contoh | Status |
|---|---|---|
| Variabel lokal | `int x = 42;` | ✅ |
| Aritmatika | `+`, `-`, `*`, `/` | ✅ |
| Perbandingan | `<=`, `>=`, `<`, `>`, `==`, `!=` | ✅ |
| Conditional | `if (n <= 1) return 1; else return n * 2;` | ✅ |
| Pemanggilan fungsi | `factorial(5)` | ✅ |
| Rekursi | `return n * factorial(n - 1);` | ✅ |
| Multi-fungsi | `f()` + `main()` dalam satu file | ✅ |
| Return dari variable | `int r = x * y; return r;` | ✅ |

### Hasil Test

| Program | Kode | Output (exit code) | Status |
|---|---|---|---|
| `test.c` | `x=42, y=10, return x+y` | **52** | ✅ |
| `test2.c` | `factorial(5)` rekursif | **120** | ✅ |
| `test_call.c` | `double_it(21)` → `n*2` | **42** | ✅ |
| `test_rec.c` | `f(3)` rekursif | **6** | ✅ |
| `test_cf.c` | `if/else` control flow | **42** | ✅ |
| `test_ret.c` | `return 42;` | **42** | ✅ |

### Test Suite Rust Core

| Modul | Hasil |
|---|---|
| `core` | 15/15 passing ✅ |
| `core/semantic` | 26/26 passing ✅ |

### Arsitektur Backend

```
┌──────────────────────────────────────────┐
│           CCVM Frontend (C++)            │
│  Lexer → Parser → Semantic → IR Gen      │
├──────────────────────────────────────────┤
│            CCVM Backend (C)              │
│  IR Parse → Instruction Select → MASM    │
├──────────────────────────────────────────┤
│         External Toolchain               │
│  ml64.exe (MASM) → link.exe (MSVC)       │
├──────────────────────────────────────────┤
│          Target: Windows x86_64          │
│  Win64 ABI, kernel32.lib, ExitProcess    │
└──────────────────────────────────────────┘
```

### Fix-Fix Kritis yang Sudah Ditangani

- **Win64 calling convention**: Parameter via `rcx, rdx, r8, r9` (bukan System V `rdi, rsi, rdx, rcx, r8, r9`)
- **Stack alignment**: `and rsp, -16` sebelum `call ExitProcess` untuk 16-byte alignment
- **`xor rax, rax` sebelum `cmp`**: Mencegah corrupt flags pada `setle`/`setg`/dll
- **Auto-load alloca variables**: IR generator otomatis insert `load` saat variabel alloca digunakan di expression
- **Pre-alokasi stack slot**: Semua locals+temps dialokasi di first pass untuk `sub rsp` yang benar
- **Immediate operands**: Binary ops dan icmp sekarang handle imm sebagai operand kedua
- **MASM syntax fixes**: `[rbp+-N]` → `[rbp-N]`, `cmp byte` → `cmp byte ptr`, dedup `extrn`
- **Short path tools**: Menggunakan 8.3 short path untuk menghindari masalah spasi di Windows

---

## Tantangan & Rencana Kedepan

### Jangka Pendek (Butuh Fix)

| # | Tantangan | Detail | Prioritas |
|---|---|---|---|
| 1 | **While/for loop** | IR generator dan backend belum support loop (hanya if/else) | 🔴 Tinggi |
| 2 | **Multi-parameter functions** | IR parser hanya handle 1 argumen per call, butuh 2+ | 🔴 Tinggi |
| 3 | **String & I/O** | Belum support string literal, `printf`, `scanf` | 🔴 Tinggi |
| 4 | **Global variables** | Hanya local variable yang bisa di-compile | 🟡 Sedang |
| 5 | **Array & pointer** | Belum ada array indexing, pointer arithmetic | 🟡 Sedang |
| 6 | **Struct/union** | Belum ada support untuk composite types | 🟡 Sedang |

### Jangka Menengah (Optimisasi)

| # | Tantangan | Detail | Prioritas |
|---|---|---|---|
| 7 | **Register allocation** | Saat ini full stack-based (`[rbp-offset]`), sangat tidak efisien | 🟡 Sedang |
| 8 | **Dead code elimination** | Instruksi setelah `ret` (`jmp t3`) masih di-generate | 🟡 Sedang |
| 9 | **mem2reg / SSA** | Alloca + store + load bisa dihilangkan untuk scalar variables | 🟡 Sedang |
| 10 | **Constant folding** | `3 + 5` seharusnya jadi `8` di compile time, bukan runtime | 🟢 Rendah |
| 11 | **Tail call optimization** | Rekursi saat ini selalu push stack frame baru | 🟢 Rendah |
| 12 | **Inline function** | Function call kecil bisa di-inline untuk performa | 🟢 Rendah |

### Jangka Panjang (Fitur Besar)

| # | Tantangan | Detail |
|---|---|---|
| 13 | **ARM64 backend** | Target macOS Apple Silicon & Windows ARM |
| 14 | **Debug info (DWARF/PDB)** | Source-level debugging dengan gdb/Visual Studio |
| 15 | **Rust semantic integration** | Sambungkan Rust core semantic analyzer ke C++ pipeline |
| 16 | **Julia integration** | Prototyping optimizer passes via Julia |
| 17 | **Self-hosting** | CCVM bisa meng-compile source CCVM sendiri |
| 18 | **Package manager** | Dependency management untuk proyek CCVM |

### Tantangan Arsitektural

| Tantangan | Status | Penjelasan |
|---|---|---|
| **Bootstrap Problem** | ⚠️ Aktif | Compiler butuh compiler untuk compile dirinya sendiri. Solusi: hybrid approach dengan fallback ke MSVC |
| **Performance Gap** | 📈 Perlu kerja | Kode yang dihasilkan saat ini stack-heavy, butuh register allocation untuk competitive performance |
| **Ecosystem Maturity** | 🌱 Awal | Belum ada package manager, IDE integration, atau community |
| **Cross-platform** | ❌ Belum | Baru Windows x86_64. ARM64, Linux, macOS masih perlu kerja besar |

---

## Arsitektur Multi-Bahasa

### Filosofi Pemilihan Bahasa
- **Rust**: Komponen inti dan manajemen memori (keamanan + performa)
- **C**: Optimasi low-level dan code generation (kontrol hardware maksimal)
- **C++**: Front-end parser dan semantic analysis (ekosistem parser kuat)
- **Julia**: Analisis numerik dan prototyping (komputasi ilmiah + metaprogramming)

### Struktur Lapisan
```
┌─────────────────────────────────────────┐
│          High-Level Layer               │
│    (Eksperimen bahasa & fitur baru)   │
├─────────────────────────────────────────┤
│          Low-Level Layer                │
│    (Deterministik & produksi-ready)   │
├─────────────────────────────────────────┤
│         Target Architecture             │
│      (x86_64, ARM64, RISC-V)          │
└─────────────────────────────────────────┘
```

## Roadmap Pengembangan 5 Tahun

### Tahun 1: Foundation (2024-2025)
- ✅ Spesifikasi IR CCVM v1.0
- ✅ Implementasi core engine Rust (41/41 tests pass)
- ✅ Parser C++ multi-bahasa
- ✅ Code generator x86_64 (Windows)
- ✅ End-to-end compilation pipeline
- 🔄 While/for loop support
- 🔄 Multi-parameter functions
- 🔄 String & I/O support

### Tahun 2: Expansion (2025-2026)
- ⏳ ARM64 & RISC-V backend
- ⏳ Semantic analyzer Rust integration
- ⏳ Register allocation
- ⏳ Optimization passes (mem2reg, DCE)
- ⏳ Test suite komprehensif

### Tahun 3: Maturation (2026-2027)
- ⏳ Julia integration
- ⏳ Debug info generation
- ⏳ Advanced optimizations
- ⏳ Performance benchmarking vs LLVM

### Tahun 4: Ecosystem (2027-2028)
- ⏳ Package manager
- ⏳ IDE integration
- ⏳ Documentation generator
- ⏳ Community building

### Tahun 5: Production (2028-2029)
- ⏳ Enterprise adoption
- ⏳ Self-hosting compiler
- ⏳ AI-assisted optimization
- ⏳ Standardization

## Struktur Proyek

```
ccvm/
├── core/                    # Rust - Engine inti & semantic analysis
├── frontend/               # C++ - Lexer, Parser, IR Generator, CLI
├── optimizer/               # C - Intermediate optimizations (TODO)
├── backend/                # C - IR Parser, x86_64 codegen
│   └── src/x86_64/         # Instruction selector (isel.c)
├── julia/                  # Julia - Numerik & prototyping (TODO)
├── tests/                  # Test suite komprehensif
├── docs/                   # Dokumentasi teknis
├── benchmarks/             # Benchmark suite (TODO)
└── tools/                  # Utilitas pendukung (TODO)
```

## Getting Started

### Prerequisites
- CMake 3.20+
- MSVC Build Tools (Windows) + MASM (`ml64.exe`)
- Windows SDK 10 (`kernel32.lib`)
- Rust 1.70+ (untuk core module)

### Build
```bash
cd frontend
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release

# Compile C source to executable
.\build\Release\ccvm-frontend-cli.exe input.c --exe -o output.exe

# View intermediate stages
.\build\Release\ccvm-frontend-cli.exe input.c --tokens   # Lexer output
.\build\Release\ccvm-frontend-cli.exe input.c --ast      # AST output
.\build\Release\ccvm-frontend-cli.exe input.c --ir       # IR output
.\build\Release\ccvm-frontend-cli.exe input.c --asm      # Assembly output
```

### Contoh
```c
// test.c
int factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

int main() {
    int result = factorial(5);
    return result;  // Exit code: 120
}
```

```bash
.\build\Release\ccvm-frontend-cli.exe test.c --exe -o factorial.exe
.\factorial.exe
echo %ERRORLEVEL%
# Output: 120
```

## Kontribusi

Kami menerima kontribusi dari komunitas dengan pedoman ketat untuk menghindari fragmentasi:

1. **Proses Review**: Semua perubahan harus melalui review oleh minimal 2 maintainer
2. **Testing**: Minimal 90% code coverage untuk semua komponen baru
3. **Dokumentasi**: Setiap fitur baru harus didokumentasikan secara komprehensif
4. **Konsistensi**: Ikuti standar pengkodean yang telah ditetapkan
5. **Kompatibilitas**: Jaga backward compatibility kecuali untuk major release

## Lisensi

MIT License - Lihat file [LICENSE](LICENSE) untuk detail.

## Kontak

- Email: team@ccvm.dev
- Discord: [CCVM Community](https://discord.gg/ccvm)
- GitHub Issues: [github.com/ccvm/ccvm/issues](https://github.com/ccvm/ccvm/issues)