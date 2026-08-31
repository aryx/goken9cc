;; Reference only: the traditional WebAssembly Text (WAT) form of the
;; exact same program as hello_wasm.s -- same WASI fd_write call, same
;; memory layout (an iovec pointing at the string, plus a scratch word
;; for the byte count fd_write reports back). Not part of the build
;; (nothing in this project reads .wat), and not run by `mk test`;
;; it exists purely so a reader can compare the two side by side and
;; see that ea/el are emitting the same wasm underneath, just reached
;; through Plan9-style assembly instead of WAT.
;;
;; See hello_wasm.s for why that file looks so different from this one
;; even though both compile to (structurally) the same module.

(module
  ;; The WASI host import. Plays the same role as hello_wasm.s's
  ;; "CALL fd_write(SB)" plus el's own
  ;; "-I fd_write=wasi_snapshot_preview1.fd_write" flag -- except WAT
  ;; requires the function's full type (four i32 params, one i32
  ;; result) spelled out right here at the import, whereas ea's
  ;; grammar never asks for one: Plan9 assembly has no type system,
  ;; so el just hardcodes this one signature instead (see
  ;; linkers/el/l.h's Import comment).
  (import "wasi_snapshot_preview1" "fd_write"
    (func $fd_write (param i32 i32 i32 i32) (result i32)))

  ;; hello_wasm.s's el also emits one page of memory and exports it
  ;; under this same name -- required by the WASI ABI, since that's
  ;; how the host and the guest module share the buffers fd_write
  ;; reads and writes.
  (memory (export "memory") 1)

  ;; iovec { ptr, len } describing the string below: 4-byte pointer
  ;; (little-endian 8) followed by a 4-byte length (little-endian 13).
  ;; The same struct as hello_wasm.s's
  ;;   DATA iov+0(SB)/4, $msg(SB)
  ;;   DATA iov+4(SB)/4, $13
  ;; just written as a raw byte string here because WAT's (data ...)
  ;; form has no equivalent of "$msg(SB)" (a linker-resolved address) --
  ;; a normal wat2wasm-style toolchain would compute this offset with
  ;; an explicit global or by hand, same as this file does.
  (data (i32.const 0) "\08\00\00\00\0d\00\00\00")

  ;; "Hello, world\n" -- same bytes as hello_wasm.s's
  ;;   DATA msg+0(SB)/8, $"Hello, w"
  ;;   DATA msg+8(SB)/5, $"orld\n"
  (data (i32.const 8) "Hello, world\0a")

  (func $_start (export "_start")
    ;; claude: WAT nests the arguments inside the call instead of
    ;; writing a push sequence, but it means exactly the same thing:
    ;; push 1, push 0, push 1, push 20, then call. This is the single
    ;; biggest surface difference from hello_wasm.s, which spells the
    ;; four pushes out as four separate MOVW lines -- see that file's
    ;; comment for why.
    (call $fd_write
      (i32.const 1)    ;; fd = 1 (stdout)
      (i32.const 0)    ;; iovs_ptr: the iovec struct above, at address 0
      (i32.const 1)    ;; iovs_len: one iovec
      (i32.const 20))  ;; nwritten_ptr: scratch word, placed after msg
    drop))             ;; discard the i32 errno fd_write returns
