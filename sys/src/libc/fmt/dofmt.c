#include <u.h>
#include <libc.h>
#include "fmtdef.h"

/* format the output into f->to and return the number of characters fmted  */
int
dofmt(Fmt *f, char *fmt)
{
	Rune rune, *rt, *rs;
	int r;
	char *t, *s;
	int n, nfmt;

	nfmt = f->nfmt;
	for(;;){
		if(f->runes){
			rt = f->to;
			rs = f->stop;
			while((r = *(uchar*)fmt) && r != '%'){
				if(r < Runeself)
					fmt++;
				else{
					fmt += chartorune(&rune, fmt);
					r = rune;
				}
				FMTRCHAR(f, rt, rs, r);
			}
			fmt++;
			f->nfmt += rt - (Rune *)f->to;
			f->to = rt;
			if(!r)
				return f->nfmt - nfmt;
			f->stop = rs;
		}else{
			t = f->to;
			s = f->stop;
			while((r = *(uchar*)fmt) && r != '%'){
				if(r < Runeself){
					FMTCHAR(f, t, s, r);
					fmt++;
				}else{
					n = chartorune(&rune, fmt);
					if(t + n > s){
						t = _fmtflush(f, t, n);
						if(t != nil)
							s = f->stop;
						else
							return -1;
					}
					while(n--)
						*t++ = *fmt++;
				}
			}
			fmt++;
			f->nfmt += t - (char *)f->to;
			f->to = t;
			if(!r)
				return f->nfmt - nfmt;
			f->stop = s;
		}

		fmt = _fmtdispatch(f, fmt, 0);
		if(fmt == nil)
			return -1;
	}
	return 0;	/* not reached */
}

void *
_fmtflush(Fmt *f, void *t, int len)
{
	if(f->runes)
		f->nfmt += (Rune*)t - (Rune*)f->to;
	else
		f->nfmt += (char*)t - (char *)f->to;
	f->to = t;
	if(f->flush == 0 || (*f->flush)(f) == 0 || (char*)f->to + len > (char*)f->stop){
		f->stop = f->to;
		return nil;
	}
	return f->to;
}

/*
 * put a formatted block of memory sz bytes long of n runes into the output buffer,
 * left/right justified in a field of at least f->width charactes
 */
int
_fmtpad(Fmt *f, int n)
{
	char *t, *s;
	int i;

	t = f->to;
	s = f->stop;
	for(i = 0; i < n; i++)
		FMTCHAR(f, t, s, ' ');
	f->nfmt += t - (char *)f->to;
	f->to = t;
	return 0;
}

int
_rfmtpad(Fmt *f, int n)
{
	Rune *t, *s;
	int i;

	t = f->to;
	s = f->stop;
	for(i = 0; i < n; i++)
		FMTRCHAR(f, t, s, ' ');
	f->nfmt += t - (Rune *)f->to;
	f->to = t;
	return 0;
}

int
_fmtcpy(Fmt *f, void *vm, int n, int sz)
{
	Rune *rt, *rs, r;
	char *t, *s, *m, *me;
	ulong fl;
	int nc, w;

	m = vm;
	me = m + sz;
	w = f->width;
	fl = f->flags;
	if((fl & FmtPrec) && n > f->prec)
		n = f->prec;
	if(f->runes){
		if(!(fl & FmtLeft) && _rfmtpad(f, w - n) < 0)
			return -1;
		rt = f->to;
		rs = f->stop;
		for(nc = n; nc > 0; nc--){
			r = *(uchar*)m;
			if(r < Runeself)
				m++;
			else if((me - m) >= UTFmax || fullrune(m, me-m))
				m += chartorune(&r, m);
			else
				break;
			FMTRCHAR(f, rt, rs, r);
		}
		f->nfmt += rt - (Rune *)f->to;
		f->to = rt;
		if(m < me)
			return -1;
		if(fl & FmtLeft && _rfmtpad(f, w - n) < 0)
			return -1;
	}else{
		if(!(fl & FmtLeft) && _fmtpad(f, w - n) < 0)
			return -1;
		t = f->to;
		s = f->stop;
		for(nc = n; nc > 0; nc--){
			r = *(uchar*)m;
			if(r < Runeself)
				m++;
			else if((me - m) >= UTFmax || fullrune(m, me-m))
				m += chartorune(&r, m);
			else
				break;
			FMTRUNE(f, t, s, r);
		}
		f->nfmt += t - (char *)f->to;
		f->to = t;
		if(fl & FmtLeft && _fmtpad(f, w - n) < 0)
			return -1;
	}
	return 0;
}

int
_fmtrcpy(Fmt *f, void *vm, int n)
{
	Rune r, *m, *me, *rt, *rs;
	char *t, *s;
	ulong fl;
	int w;

	m = vm;
	w = f->width;
	fl = f->flags;
	if((fl & FmtPrec) && n > f->prec)
		n = f->prec;
	if(f->runes){
		if(!(fl & FmtLeft) && _rfmtpad(f, w - n) < 0)
			return -1;
		rt = f->to;
		rs = f->stop;
		for(me = m + n; m < me; m++)
			FMTRCHAR(f, rt, rs, *m);
		f->nfmt += rt - (Rune *)f->to;
		f->to = rt;
		if(fl & FmtLeft && _rfmtpad(f, w - n) < 0)
			return -1;
	}else{
		if(!(fl & FmtLeft) && _fmtpad(f, w - n) < 0)
			return -1;
		t = f->to;
		s = f->stop;
		for(me = m + n; m < me; m++){
			r = *m;
			FMTRUNE(f, t, s, r);
		}
		f->nfmt += t - (char *)f->to;
		f->to = t;
		if(fl & FmtLeft && _fmtpad(f, w - n) < 0)
			return -1;
	}
	return 0;
}

/* fmt out one character */
int
_charfmt(Fmt *f)
{
	char x[1];

	x[0] = va_arg(f->args, int);
	return _fmtcpy(f, x, 1, 1);
}

/* fmt out one rune */
int
_runefmt(Fmt *f)
{
	Rune x[1];

	x[0] = va_arg(f->args, int);
	return _fmtrcpy(f, x, 1);
}

/* public helper routine: fmt out a null terminated string already in hand */
int
fmtstrcpy(Fmt *f, char *s)
{
	if(!s)
		return _fmtcpy(f, "<nil>", 5, 5);
	return _fmtcpy(f, s, utflen(s), strlen(s));
}

/* fmt out a null terminated utf string */
int
_strfmt(Fmt *f)
{
	char *s;

	s = va_arg(f->args, char *);
	return fmtstrcpy(f, s);
}

/* public helper routine: fmt out a null terminated rune string already in hand */
int
fmtrunestrcpy(Fmt *f, Rune *s)
{
	Rune *e;
	int n;

	if(!s)
		return _fmtcpy(f, "<nil>", 5, 5);
	for(e = s; *e; e++)
		;
	n = e - s;
	if((f->flags & FmtPrec) && n > f->prec)
		n = f->prec;
	return _fmtrcpy(f, s, n);
}

/* fmt out a null terminated rune string */
int
_runesfmt(Fmt *f)
{
	Rune *s;

	s = va_arg(f->args, Rune *);
	return fmtrunestrcpy(f, s);
}

/* fmt a % */
int
_percentfmt(Fmt *f)
{
	Rune x[1];

	x[0] = f->r;
	return _fmtrcpy(f, x, 1);
}

/* fmt an integer */
int
_ifmt(Fmt *f)
{
	char buf[70], *p, *conv;
	uvlong vu;
	ulong u;
	int neg, base, i, n, fl, w, isv;

	neg = 0;
	fl = f->flags;
	isv = 0;
	vu = 0;
	u = 0;
	if(f->r == 'p'){
		u = (ulong)va_arg(f->args, void*);
		f->r = 'x';
		fl |= FmtUnsigned;
	}else if(fl & FmtVLong){
		isv = 1;
		if(fl & FmtUnsigned)
			vu = va_arg(f->args, uvlong);
		else
			vu = va_arg(f->args, vlong);
	}else if(fl & FmtLong){
		if(fl & FmtUnsigned)
			u = va_arg(f->args, ulong);
		else
			u = va_arg(f->args, long);
	}else if(fl & FmtShort){
		if(fl & FmtUnsigned)
			u = va_arg(f->args, ushort);
		else
			u = va_arg(f->args, short);
	}else{
		if(fl & FmtUnsigned)
			u = va_arg(f->args, uint);
		else
			u = va_arg(f->args, int);
	}
	conv = "0123456789abcdef";
	switch(f->r){
	case 'd':
		base = 10;
		break;
	case 'x':
		base = 16;
		break;
	case 'X':
		base = 16;
		conv = "0123456789ABCDEF";
		break;
	case 'b':
		base = 2;
		break;
	case 'o':
		base = 8;
		break;
	default:
		return -1;
	}
	if(!(fl & FmtUnsigned)){
		if(isv && (vlong)vu < 0){
			vu = -(vlong)vu;
			neg = 1;
		}else if(!isv && (long)u < 0){
			u = -(long)u;
			neg = 1;
		}
	}
	p = buf + sizeof buf - 1;
	n = 0;
	if(isv){
		while(vu){
			i = vu % base;
			vu /= base;
			if((fl & FmtComma) && n % 4 == 3){
				*p-- = ',';
				n++;
			}
			*p-- = conv[i];
			n++;
		}
	}else{
		while(u){
			i = u % base;
			u /= base;
			if((fl & FmtComma) && n % 4 == 3){
				*p-- = ',';
				n++;
			}
			*p-- = conv[i];
			n++;
		}
	}
	if(n == 0){
		*p-- = '0';
		n = 1;
	}
	for(w = f->prec; n < w && p > buf+3; n++)
		*p-- = '0';
	if(neg || (fl & (FmtSign|FmtSpace)))
		n++;
	if(fl & FmtSharp){
		if(base == 16)
			n += 2;
		else if(base == 8){
			if(p[1] == '0')
				fl &= ~FmtSharp;
			else
				n++;
		}
	}
	if((fl & FmtZero) && !(fl & FmtLeft)){
		for(w = f->width; n < w && p > buf+3; n++)
			*p-- = '0';
		f->width = 0;
	}
	if(fl & FmtSharp){
		if(base == 16)
			*p-- = f->r;
		if(base == 16 || base == 8)
			*p-- = '0';
	}
	if(neg)
		*p-- = '-';
	else if(fl & FmtSign)
		*p-- = '+';
	else if(fl & FmtSpace)
		*p-- = ' ';
	f->flags &= ~FmtPrec;
	return _fmtcpy(f, p + 1, n, n);
}

int
_countfmt(Fmt *f)
{
	void *p;
	ulong fl;

	fl = f->flags;
	p = va_arg(f->args, void*);
	if(fl & FmtVLong){
		*(vlong*)p = f->nfmt;
	}else if(fl & FmtLong){
		*(long*)p = f->nfmt;
	}else if(fl & FmtByte){
		*(char*)p = f->nfmt;
	}else if(fl & FmtShort){
		*(short*)p = f->nfmt;
	}else{
		*(int*)p = f->nfmt;
	}
	return 0;
}

int
_flagfmt(Fmt *f)
{
	switch(f->r){
	case ',':
		f->flags |= FmtComma;
		break;
	case '-':
		f->flags |= FmtLeft;
		break;
	case '+':
		f->flags |= FmtSign;
		break;
	case '#':
		f->flags |= FmtSharp;
		break;
	case ' ':
		f->flags |= FmtSpace;
		break;
	case 'u':
		f->flags |= FmtUnsigned;
		break;
	case 'h':
		if(f->flags & FmtShort)
			f->flags |= FmtByte;
		f->flags |= FmtShort;
		break;
	case 'l':
		if(f->flags & FmtLong)
			f->flags |= FmtVLong;
		f->flags |= FmtLong;
		break;
	}
	return 1;
}

/* default error format */
int
_badfmt(Fmt *f)
{
	char x[3];

	x[0] = '%';
	x[1] = f->r;
	x[2] = '%';
	_fmtcpy(f, x, 3, 3);
	return 0;
}
