#pragma once

#pragma function(memcpy)
inline void *memcpy(void *dest, const void *src, size_t count)
{
	char *dest8 = static_cast<char *>(dest);
	const char *src8 = static_cast<const char *>(src);
	while (count--)
		*dest8++ = *src8++;
	return dest;
}

#pragma function(wcslen)
inline size_t wcslen(const wchar_t* string)
{
	const wchar_t* scan;
	for (scan = string; *scan; ++scan) {}
	return static_cast<size_t>(scan - string);
}
