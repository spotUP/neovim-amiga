/* <wctype.h> for ixemul on the Unicode database utf8proc already links
 * into Neovim 0.12 (libamigawide.a, linked before libutf8proc). ixemul has
 * no wide-character classification; libamigacompat's <wctype.h> declares
 * these. wint_t is a Unicode code point here, as in Neovim's UTF-8 world. */
#include <wchar.h>
#include <wctype.h>
#include "utf8proc.h"

static utf8proc_category_t
cat(wint_t c)
{
	if (c < 0 || c > 0x10FFFF)
		return UTF8PROC_CATEGORY_CN;
	return utf8proc_category((utf8proc_int32_t)c);
}

int
iswalpha(wint_t c)
{
	switch (cat(c)) {
	case UTF8PROC_CATEGORY_LU: case UTF8PROC_CATEGORY_LL:
	case UTF8PROC_CATEGORY_LT: case UTF8PROC_CATEGORY_LM:
	case UTF8PROC_CATEGORY_LO:
		return 1;
	default:
		return 0;
	}
}

int
iswdigit(wint_t c)
{
	return c >= '0' && c <= '9';	/* POSIX: the decimal digits only */
}

int
iswalnum(wint_t c)
{
	switch (cat(c)) {
	case UTF8PROC_CATEGORY_ND: case UTF8PROC_CATEGORY_NL:
	case UTF8PROC_CATEGORY_NO:
		return 1;
	default:
		return iswalpha(c);
	}
}

int
iswspace(wint_t c)
{
	if (c == ' ' || (c >= '\t' && c <= '\r'))
		return 1;
	switch (cat(c)) {
	case UTF8PROC_CATEGORY_ZS: case UTF8PROC_CATEGORY_ZL:
	case UTF8PROC_CATEGORY_ZP:
		return c != 0x00A0 && c != 0x2007 && c != 0x202F;  /* no-break */
	default:
		return 0;
	}
}

int
iswupper(wint_t c)
{
	return cat(c) == UTF8PROC_CATEGORY_LU;
}

int
iswlower(wint_t c)
{
	return cat(c) == UTF8PROC_CATEGORY_LL;
}

int
iswcntrl(wint_t c)
{
	return cat(c) == UTF8PROC_CATEGORY_CC;
}

int
iswpunct(wint_t c)
{
	switch (cat(c)) {
	case UTF8PROC_CATEGORY_PC: case UTF8PROC_CATEGORY_PD:
	case UTF8PROC_CATEGORY_PS: case UTF8PROC_CATEGORY_PE:
	case UTF8PROC_CATEGORY_PI: case UTF8PROC_CATEGORY_PF:
	case UTF8PROC_CATEGORY_PO: case UTF8PROC_CATEGORY_SM:
	case UTF8PROC_CATEGORY_SC: case UTF8PROC_CATEGORY_SK:
	case UTF8PROC_CATEGORY_SO:
		return 1;
	default:
		return 0;
	}
}

int
iswprint(wint_t c)
{
	switch (cat(c)) {
	case UTF8PROC_CATEGORY_CN: case UTF8PROC_CATEGORY_CC:
	case UTF8PROC_CATEGORY_CS: case UTF8PROC_CATEGORY_ZL:
	case UTF8PROC_CATEGORY_ZP:
		return 0;
	default:
		return 1;
	}
}

int
iswgraph(wint_t c)
{
	return iswprint(c) && cat(c) != UTF8PROC_CATEGORY_ZS;
}

int
iswxdigit(wint_t c)
{
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
	    (c >= 'A' && c <= 'F');
}

wint_t
towupper(wint_t c)
{
	if (c < 0 || c > 0x10FFFF)
		return c;
	return (wint_t)utf8proc_toupper((utf8proc_int32_t)c);
}

wint_t
towlower(wint_t c)
{
	if (c < 0 || c > 0x10FFFF)
		return c;
	return (wint_t)utf8proc_tolower((utf8proc_int32_t)c);
}
