
// enum Namespace_flag, mount/bind parameter
#define	MREPL	0x0000	/* mount replaces object */
#define	MBEFORE	0x0001	/* mount goes before others in union directory */
#define	MAFTER	0x0002	/* mount goes after others in union directory */

#define	MCREATE	0x0004	/* permit creation in mounted directory */
#define	MCACHE	0x0010	/* cache some data */
// bitset<Namespace_flag>
#define	MORDER	0x0003	/* mask for bits defining order of mounting */

#define	MMASK	0x0017	/* all bits on */

extern	int	bind(char*, char*, int/*Mxxx*/);
extern	int	mount(fdt, int, char*, int/*Mxxx*/, char*);
extern	int	unmount(char*, char*);
