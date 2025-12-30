/*maqueos库函数---------------------------------------------------------------------*/
static inline void write_csr_32(unsigned int val, unsigned int csr)
{
	asm volatile("csrwr %0, %1"
							 :
							 : "r"(val), "i"(csr));
}

static inline unsigned int read_csr_32(unsigned int csr)
{
	unsigned int val;

	asm volatile("csrrd %0, %1"
							 : "=r"(val)
							 : "i"(csr));
	return val;
}

static inline void write_csr_64(unsigned long val, unsigned int csr)
{
	asm volatile("csrwr %0, %1"
							 :
							 : "r"(val), "i"(csr));
}

static inline unsigned long read_csr_64(unsigned int csr)
{
	unsigned long val;

	asm volatile("csrrd %0, %1"
							 : "=r"(val)
							 : "i"(csr));
	return val;
}

static inline void write_iocsr(unsigned long val, unsigned long reg)
{
	asm volatile("iocsrwr.d %0, %1"
							 :
							 : "r"(val), "r"(reg));
}

static inline unsigned long read_iocsr(unsigned long reg)
{
	unsigned long val;

	asm volatile("iocsrrd.d %0, %1"
							 : "=r"(val)
							 : "r"(reg));
	return val;
}

static inline unsigned int read_cpucfg(int cfg_num)
{
	unsigned int val;

	asm volatile("cpucfg %0, %1"
							 : "=r"(val)
							 : "r"(cfg_num));
	return val;
}

static inline void invalidate()
{
	asm volatile("invtlb 0x0,$r0,$r0");
}

static inline void set_mem(char *to, int c, int nr)
{
	for (int i = 0; i < nr; i++)
		to[i] = c;
}

static inline void copy_mem(char *to, char *from, int nr)
{
	for (int i = 0; i < nr; i++)
		to[i] = from[i];
}

static inline void copy_string(char *to, char *from)
{
	int nr = 0;

	while (from[nr++] != '\0')
		;
	copy_mem(to, from, nr);
}

static inline int match(char *str1, char *str2, int nr)
{
	for (int i = 0; i < nr; i++)
	{
		if (str1[i] != str2[i])
			return 0;
		if (str1[i] == '\0')
			return 1;
	}
	return 0;
}