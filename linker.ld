SECTIONS
{
  . = 0x4e000000;
  _hend = . ;
  .text : {
    *boot.o(.text)
    *(.text)
    . = ALIGN(4);
    __preinit_array_start = .;
    KEEP (*(.preinit_array))
    __preinit_array_end = .;

    . = ALIGN(4);
    __init_array_start = .;
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array))
    __init_array_end = .;
  }
  .ARM.extab   : { *(.ARM.extab* .gnu.linkonce.armextab.*) }
   PROVIDE_HIDDEN (__exidx_start = .);
  .ARM.exidx   : { *(.ARM.exidx* .gnu.linkonce.armexidx.*) }
   PROVIDE_HIDDEN (__exidx_end = .);
  .data :  {
    *(.data)
  }
  .bss : {
    _bstart1 = . ;
    *(.bss); *(COMMON);
    _bend1 = . ;
  }
  .uncached 0x50000000 : {
    _bstart2 = . ;
    *(UNCACHED);
    _bend2 = . ;
  }
}
