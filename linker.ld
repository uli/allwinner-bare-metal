SECTIONS
{
  . = 0x4e000000;
  _hend = . ;
  .text : {
    boot.o(.text)
    *(.text)
  }
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
