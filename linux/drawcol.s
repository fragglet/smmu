// Emacs style mode select -*- asm -*-
//-------------------------------------------------------------------------
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//--------------------------------------------------------------------------


//================
//
// R_DrawColumn
//
//================
//
// 2/15/98 Lee Killough
//
// Converted C code with TFE fix to assembly and tuned
//
// 2/21/98 killough: added translucency support
//
// 11/98 killough: added hires support
//
//================

#ifdef I386
	
 .text
 .align 8
 .globl R_DrawColumn
R_DrawColumn:
 pushl %ebp
 pushl %esi
 pushl %edi
 pushl %ebx
 movl dc_yh,%esi
 movl dc_yl,%edx
 movl dc_x,%eax
 incl %esi
 movl ylookup(,%edx,4),%ebx
 subl %edx,%esi
 jle end
 addl columnofs(,%eax,4),%ebx
 movl dc_texheight,%eax
 subl centery,%edx
 movl dc_source,%ebp
 imull dc_iscale,%edx
 leal -1(%eax),%ecx
 movl dc_colormap,%edi
 addl dc_texturemid,%edx

 cmpl $0, hires
 jne hi

 testl %eax,%ecx
 je powerof2
 sall $16,%eax

red1:
 subl %eax,%edx
 jge red1

red2:
 addl %eax,%edx
 jl red2

 .align 8,0x90
nonp2loop:
 movl %edx,%ecx
 sarl $16,%ecx
 addl dc_iscale,%edx
 movzbl (%ecx,%ebp),%ecx
 movb (%edi,%ecx),%cl
 movb %cl,(%ebx)
 addl $320,%ebx
 cmpl %eax,%edx
 jge wraparound
 decl %esi
 jg nonp2loop
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

 .align 8
wraparound:
 subl %eax,%edx
 decl %esi
 jg nonp2loop
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

 .align 8
end:
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

 .align 8
p2loop:
 movl %edx,%eax
 sarl $16,%eax
 addl dc_iscale,%edx
 andl %ecx,%eax
 movzbl (%eax,%ebp),%eax
 movb (%eax,%edi),%al
 movb %al,(%ebx)
 movl %edx,%eax
 addl dc_iscale,%edx
 sarl $16,%eax
 andl %ecx,%eax
 movzbl (%eax,%ebp),%eax
 movb (%eax,%edi),%al
 movb %al,320(%ebx)
 addl $640,%ebx

powerof2:
 addl $-2,%esi
 jge p2loop
 jnp end
 sarl $16,%edx
 andl %ecx,%edx
 xorl %eax,%eax
 movb (%edx,%ebp),%al
 movb (%eax,%edi),%al
 movb %al,(%ebx)
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

// killough 11/98: add hires support

 .align 8
hi:
 testl %eax,%ecx
 je powerof2_hi
 sall $16,%eax

red1_hi:
 subl %eax,%edx
 jge red1_hi

red2_hi:
 addl %eax,%edx
 jl red2_hi

 .align 8,0x90
nonp2loop_hi:
 movl %edx,%ecx
 sarl $16,%ecx
 addl dc_iscale,%edx
 movzbl (%ecx,%ebp),%ecx
 movb (%edi,%ecx),%cl
 movb %cl,(%ebx)
 addl $640,%ebx
 cmpl %eax,%edx
 jge wraparound_hi
 decl %esi
 jg nonp2loop_hi
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

 .align 8
wraparound_hi:
 subl %eax,%edx
 decl %esi
 jg nonp2loop_hi
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

 .align 8
p2loop_hi:
 movl %edx,%eax
 sarl $16,%eax
 addl dc_iscale,%edx
 andl %ecx,%eax
 movzbl (%eax,%ebp),%eax
 movb (%eax,%edi),%al
 movb %al,(%ebx)
 movl %edx,%eax
 addl dc_iscale,%edx
 sarl $16,%eax
 andl %ecx,%eax
 movzbl (%eax,%ebp),%eax
 addl $1280,%ebx
 movb (%eax,%edi),%al
 movb %al,-640(%ebx)

powerof2_hi:
 addl $-2,%esi
 jge p2loop_hi
 jnp end
 sarl $16,%edx
 andl %ecx,%edx
 xorl %eax,%eax
 movb (%edx,%ebp),%al
 movb (%eax,%edi),%al
 movb %al,(%ebx)
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

//================
//
// R_DrawTLColumn
//
// Translucency support
//
//================

 .align 8
 .globl R_DrawTLColumn
R_DrawTLColumn:

 pushl %ebp
 pushl %esi
 pushl %edi
 pushl %ebx
 movl dc_yh,%esi
 movl dc_yl,%edx
 movl dc_x,%eax
 incl %esi
 movl ylookup(,%edx,4),%ebx
 subl %edx,%esi
 jle end_tl
 addl columnofs(,%eax,4),%ebx
 movl dc_texheight,%eax
 subl centery,%edx
 movl dc_source,%ebp
 imull dc_iscale,%edx
 leal -1(%eax),%ecx
 movl dc_colormap,%edi
 addl dc_texturemid,%edx

 cmpl $0, hires
 jne hi_tl

 testl %eax,%ecx
 pushl %ecx
 je powerof2_tl
 sall $16,%eax

red1_tl:
 subl %eax,%edx
 jge red1_tl

red2_tl:
 addl %eax,%edx
 jl red2_tl
 pushl %esi

 .align 8,0x90
nonp2loop_tl:
 xorl %ecx,%ecx
 movl %edx,%esi
 movb (%ebx),%cl
 shll $8,%ecx
 sarl $16,%esi
 addl tranmap,%ecx
 addl dc_iscale,%edx
 movzbl (%esi,%ebp),%esi
 movzbl (%edi,%esi),%esi
 movb (%ecx,%esi),%cl
 movb %cl,(%ebx)
 addl $320,%ebx
 cmpl %eax,%edx
 jge wraparound_tl
 decl (%esp)
 jg nonp2loop_tl
 popl %eax
 popl %ecx
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

 .align 8
wraparound_tl:
 subl %eax,%edx
 decl (%esp)
 jg nonp2loop_tl
 popl %eax
 popl %ecx
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

 .align 8
end_tl:
 popl %ecx
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

 .align 8
p2loop_tl:
 movl %edx,%eax
 xorl %ecx,%ecx
 addl dc_iscale,%edx
 movb (%ebx),%cl
 sarl $16,%eax
 shll $8,%ecx
 andl (%esp),%eax
 addl tranmap,%ecx
 movzbl (%eax,%ebp),%eax
 movzbl (%edi,%eax),%eax
 movb   (%ecx,%eax),%al
 xorl %ecx,%ecx
 movb %al,(%ebx)
 movb 320(%ebx),%cl
 movl %edx,%eax
 addl dc_iscale,%edx
 sarl $16,%eax
 shll $8,%ecx
 andl (%esp),%eax
 addl tranmap,%ecx
 movzbl (%eax,%ebp),%eax
 movzbl (%edi,%eax),%eax
 movb   (%ecx,%eax),%al
 movb %al,320(%ebx)
 addl $640,%ebx

powerof2_tl:
 addl $-2,%esi
 jge p2loop_tl
 jnp end_tl
 xorl %ecx,%ecx
 sarl $16,%edx
 movb (%ebx),%cl
 andl (%esp),%edx
 shll $8,%ecx
 xorl %eax,%eax
 addl tranmap,%ecx
 movb (%edx,%ebp),%al
 movzbl (%eax,%edi),%eax
 movb (%ecx,%eax),%al 
 movb %al,(%ebx)
 popl %ecx
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

// killough 11/98: add hires support

hi_tl:

 testl %eax,%ecx
 pushl %ecx
 je powerof2_tl_hi
 sall $16,%eax

red1_tl_hi:
 subl %eax,%edx
 jge red1_tl_hi

red2_tl_hi:
 addl %eax,%edx
 jl red2_tl_hi
 pushl %esi

 .align 8,0x90
nonp2loop_tl_hi:
 xorl %ecx,%ecx
 movl %edx,%esi
 movb (%ebx),%cl
 shll $8,%ecx
 sarl $16,%esi
 addl tranmap,%ecx
 addl dc_iscale,%edx
 movzbl (%esi,%ebp),%esi
 movzbl (%edi,%esi),%esi
 movb (%ecx,%esi),%cl
 movb %cl,(%ebx)
 addl $640,%ebx
 cmpl %eax,%edx
 jge wraparound_tl_hi
 decl (%esp)
 jg nonp2loop_tl_hi
 popl %eax
 popl %ecx
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

 .align 8
wraparound_tl_hi:
 subl %eax,%edx
 decl (%esp)
 jg nonp2loop_tl_hi
 popl %eax
 popl %ecx
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

 .align 8
p2loop_tl_hi:
 movl %edx,%eax
 xorl %ecx,%ecx
 addl dc_iscale,%edx
 movb (%ebx),%cl
 sarl $16,%eax
 shll $8,%ecx
 andl (%esp),%eax
 addl tranmap,%ecx
 movzbl (%eax,%ebp),%eax
 movzbl (%edi,%eax),%eax
 movb   (%ecx,%eax),%al
 xorl %ecx,%ecx
 movb %al,(%ebx)
 movb 640(%ebx),%cl
 movl %edx,%eax
 addl dc_iscale,%edx
 sarl $16,%eax
 shll $8,%ecx
 andl (%esp),%eax
 addl tranmap,%ecx
 movzbl (%eax,%ebp),%eax
 movzbl (%edi,%eax),%eax
 movb   (%ecx,%eax),%al
 movb %al,640(%ebx)
 addl $1280,%ebx

powerof2_tl_hi:
 addl $-2,%esi
 jge p2loop_tl_hi
 jnp end_tl
 xorl %ecx,%ecx
 sarl $16,%edx
 movb (%ebx),%cl
 andl (%esp),%edx
 shll $8,%ecx
 xorl %eax,%eax
 addl tranmap,%ecx
 movb (%edx,%ebp),%al
 movzbl (%eax,%edi),%eax
 movb (%ecx,%eax),%al 
 movb %al,(%ebx)
 popl %ecx
 popl %ebx
 popl %edi
 popl %esi
 popl %ebp
 ret

#endif /* #ifdef USEASM */
	
//----------------------------------------------------------------------------
//
// $Log$
// Revision 1.2  2001-01-13 02:13:13  fraggle
// Change USEASM #define to I386
//
// Revision 1.1.1.1  2000/04/30 19:12:09  fraggle
// initial import
//
//
//----------------------------------------------------------------------------
