#ifndef _ASM_ARM_ARCH_RESERVED_MEM_H
#define _ASM_ARM_ARCH_RESERVED_MEM_H


/*
 * Defualt reserved memory size
 * MFC		: 6 MB
 * Post		: 8 MB
 * JPEG		: 8 MB
 * CMM		: 8 MB
 * TV		: 8 MB
 * Camera	: 15 MB
 * These sizes can be modified
 */


//#define CONFIG_RESERVED_MEM_JPEG
//#define CONFIG_RESERVED_MEM_JPEG_POST
//#define CONFIG_RESERVED_MEM_MFC
//#define CONFIG_RESERVED_MEM_MFC_POST
//#define CONFIG_RESERVED_MEM_JPEG_MFC_POST
//#define CONFIG_RESERVED_MEM_JPEG_CAMERA
//#define CONFIG_RESERVED_MEM_JPEG_POST_CAMERA
//#define CONFIG_RESERVED_MEM_MFC_CAMERA
//#define CONFIG_RESERVED_MEM_MFC_POST_CAMERA
//#define CONFIG_RESERVED_MEM_JPEG_MFC_POST_CAMERA
//#define CONFIG_RESERVED_MEM_CMM_MFC_POST
//#define CONFIG_RESERVED_MEM_CMM_JPEG_MFC_POST_CAMERA
//#define CONFIG_RESERVED_MEM_TV_MFC_POST_CAMERA

#if defined(CONFIG_RESERVED_MEM_JPEG)
#define JPEG_RESERVED_MEM_START		0x57800000

#elif defined(CONFIG_RESERVED_MEM_JPEG_POST)	
#define JPEG_RESERVED_MEM_START		0x57000000
#define POST_RESERVED_MEM_START		0x57800000

#elif defined(CONFIG_RESERVED_MEM_MFC)			
#define MFC_RESERVED_MEM_START		0x57A00000

#elif defined(CONFIG_RESERVED_MEM_MFC_POST)
#define MFC_RESERVED_MEM_START		0x57200000
#define POST_RESERVED_MEM_START		0x57800000

#elif defined(CONFIG_RESERVED_MEM_JPEG_MFC_POST)
#define JPEG_RESERVED_MEM_START		0x56A00000
#define MFC_RESERVED_MEM_START		0x57200000
#define POST_RESERVED_MEM_START		0x57800000

#elif defined(CONFIG_RESERVED_MEM_JPEG_CAMERA)
#define JPEG_RESERVED_MEM_START		0x56900000

#elif defined(CONFIG_RESERVED_MEM_JPEG_POST_CAMERA)
#define JPEG_RESERVED_MEM_START		0x56100000
#define POST_RESERVED_MEM_START		0x56900000

#elif defined(CONFIG_RESERVED_MEM_MFC_CAMERA)
#define MFC_RESERVED_MEM_START		0x56B00000

#elif defined(CONFIG_RESERVED_MEM_MFC_POST_CAMERA)
#define MFC_RESERVED_MEM_START		0x56300000
#define POST_RESERVED_MEM_START		0x56900000

#elif defined(CONFIG_RESERVED_MEM_JPEG_MFC_POST_CAMERA)
#define JPEG_RESERVED_MEM_START		0x55B00000
#define MFC_RESERVED_MEM_START		0x56300000
#define POST_RESERVED_MEM_START		0x56900000

#elif defined (CONFIG_RESERVED_MEM_CMM_MFC_POST)
#define CMM_RESERVED_MEM_START		0x56A00000
#define MFC_RESERVED_MEM_START		0x57200000
#define POST_RESERVED_MEM_START		0x57800000

#elif defined (CONFIG_RESERVED_MEM_CMM_JPEG_MFC_POST_CAMERA)
#define CMM_RESERVED_MEM_START		0x55300000
#define JPEG_RESERVED_MEM_START		0x55B00000
#define MFC_RESERVED_MEM_START		0x56300000
#define POST_RESERVED_MEM_START		0x56900000

#elif defined (CONFIG_RESERVED_MEM_TV_MFC_POST_CAMERA)
#define TV_RESERVED_MEM_START		0x55B00000
#define MFC_RESERVED_MEM_START		0x56300000
#define POST_RESERVED_MEM_START		0x56900000

#endif


#endif /* _ASM_ARM_ARCH_RESERVED_MEM_H */

