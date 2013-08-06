
   .globl missBoxArt
   .balign 32
   missBoxArt:
   .incbin	"./main/boxart_none.tex"
   .globl missBoxArt_length
   missBoxArt_length:
   .long (missBoxArt_length - missBoxArt)
      
