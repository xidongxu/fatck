// fatck.c : fat file system check tools source file
#include "fatck.h"

// FAT12/16/32 common field (offset from 0 to 35)
#define BS_JMPBOOT          (0)
#define BS_OEMNAME          (3)
#define BPB_BYTSPERSEC      (11)
#define BPB_SECPERCLUS      (13)
#define BPB_RSVDSECCNT      (14)
#define BPB_NUMFATS         (16)
#define BPB_ROOTENTCNT      (17)
#define BPB_TOTSEC16        (19)
#define BPB_MEDIA           (21)
#define BPB_FATSZ16         (22)
#define BPB_SECPERTRK       (24)
#define BPB_NUMHEADS        (26)
#define BPB_HIDDSEC         (28)
#define BPB_TOTSEC32        (32)
// Fields for FAT12/16 volumes (offset from 36)
#define BS_DRVNUM           (36)
#define BS_RESERVED1        (37)
#define BS_BOOTSIG          (38)
#define BS_VOLID            (39)
#define BS_VOLLAB           (43)
#define BS_FILSYSTYPE       (54)
// Fields for FAT32 volumes (offset from 36)
#define BPB_FATSZ32         (36)
#define BPB_EXTFLAGS        (40)
#define BPB_FSVER           (42)
#define BPB_ROOTCLUS        (44)
#define BPB_FSINFO          (48)
#define BPB_BKBOOTSEC       (50)
#define BS_32_DRVNUM        (64)
#define BS_32_BOOTSIG       (66)
#define BS_32_VOLID         (67)
#define BS_32_VOLLAB        (71)
#define BS_32_FILSYSTYPE    (82)
// FAT12/16/32 common field (offset from 510)
#define BPB_BOOT_SIG        (510)

#ifndef FAT_DPT_ADDRESS
#define FAT_DPT_ADDRESS     (0x1BE)
#endif
#ifndef FAT_DIR_ENTRY_SIZE
#define FAT_DIR_ENTRY_SIZE  (32)
#endif

#define FAT_TYPE_FAT12      (12)
#define FAT_TYPE_FAT16      (16)
#define FAT_TYPE_FAT32      (32)

#define FAT_FSINFO_FREECNT  (488)
#define FAT_FSINFO_NEXTFREE (492)

#define FAT_GET_UINT16(x)   ((*(x)) | (*((x) + 1) << 8))
#define FAT_GET_UINT32(x)   ((uint32_t)*((x) + 0) << 0x00) | \
                            ((uint32_t)*((x) + 1) << 0x08) | \
                            ((uint32_t)*((x) + 2) << 0x10) | \
                            ((uint32_t)*((x) + 3) << 0x18)

typedef struct fat_bpb 
{
    // FAT12/16/32 common field (offset from 0 to 35)
    uint8_t  BS_JmpBoot[3];
    uint8_t  BS_OEMName[9];
    uint16_t BPB_BytsPerSec;
    uint8_t  BPB_SecPerClus;
    uint16_t BPB_RsvdSecCnt;
    uint8_t  BPB_NumFATs;
    uint16_t BPB_RootEntCnt;
    uint16_t BPB_TotSec16;
    uint8_t  BPB_Media;
    uint16_t BPB_FATSz16;
    uint16_t BPB_SecPerTrk;
    uint16_t BPB_NumHeads;
    uint32_t BPB_HiddSec;
    uint32_t BPB_TotSec32;

    // Fields for FAT12/16 volumes (offset from 36)
    uint8_t  BS_DrvNum;
    uint8_t  BS_Reserved;
    uint8_t  BS_BootSig;
    uint32_t BS_VolID;
    uint8_t  BS_VolLab[12];
    uint8_t  BS_FilSysType[9];
    uint8_t  BS_BootCode[448];
    uint16_t BS_BootSign;

    // Fields for FAT32 volumes (offset from 36)
    uint32_t BPB_FATSz32;
    uint16_t BPB_ExtFlags;
    uint16_t BPB_FSVer;
    uint32_t BPB_RootClus;
    uint16_t BPB_FSInfo;
    uint16_t BPB_BkBootSec;
    uint8_t  BPB_Reserved[13];
} fat_bpb_t;

typedef struct fat_fs 
{
    fat_bpb_t bpb;
    uint8_t  fat_type;
    uint8_t  fat_bits;
    uint32_t fat_size;
    uint8_t  root_dir_sectors;
    uint32_t total_sectors;
    uint32_t root_dir_sector;
    uint32_t first_data_sector;
    uint32_t start_sector;
    uint32_t data_clusters;
    uint32_t free_count;
    uint32_t next_free;
} fat_fs_t;

#define first_sector_of_cluster(fatfs, cluster) (((cluster)-2) * (fatfs)->bpb.BPB_SecPerClus + (fatfs)->first_data_sector)

static int fat_root_read(fatdev_t* device)
{
    int result = -1;
    uint8_t dpt_addr = 0;
    uint8_t* sec_bpb = NULL;
    fat_fs_t* fatfs = (fat_fs_t*)calloc(1, sizeof(fat_fs_t));
    fat_bpb_t* bpb = &fatfs->bpb;
    // get fat device sector count.
    device->sector_count = device->file_size / device->sector_size;
    // get fat device block size.
    device->sector_buff = (uint8_t*)calloc(1, device->sector_size * sizeof(uint8_t));
    if (device->sector_buff == NULL)
    {
        printf("fat device sector buffer malloc failed.\r\n");
        return result;
    }
    result = fatdev_read(device, 0, device->sector_buff, device->sector_size);
    if (result != device->sector_size)
    {
        printf("fat root get dpt address failed.\r\n");
        return result;
    }
    // get fat device start parttion.
    dpt_addr = device->sector_buff[FAT_DPT_ADDRESS];
    if ((dpt_addr != 0x80) && (dpt_addr != 0x00))
    {
        device->part_start = 0;
    }
    else
    {
        device->part_start = FAT_GET_UINT32(&device->sector_buff[FAT_DPT_ADDRESS] + 8);
    }
    result = fatdev_read(device, (device->part_start * device->sector_size), device->sector_buff, device->sector_size);
    if (result != device->sector_size)
    {
        printf("fat root read start parttion failed.\r\n");
        return result;
    }
    // get fat device bpb sector.
    sec_bpb = device->sector_buff;
    // parse the bpb information.
    strncpy(bpb->BS_OEMName, (char*)&sec_bpb[BS_OEMNAME], (sizeof(bpb->BS_OEMName) - 1));
    bpb->BS_OEMName[8] = 0;
    bpb->BPB_BytsPerSec = FAT_GET_UINT16(&sec_bpb[BPB_BYTSPERSEC]);
    bpb->BPB_SecPerClus = sec_bpb[BPB_SECPERCLUS];
    bpb->BPB_RsvdSecCnt = FAT_GET_UINT16(&sec_bpb[BPB_RSVDSECCNT]);
    bpb->BPB_NumFATs = sec_bpb[BPB_NUMFATS];
    bpb->BPB_TotSec16 = FAT_GET_UINT16(&sec_bpb[BPB_TOTSEC16]);
    bpb->BPB_Media = sec_bpb[BPB_MEDIA];
    bpb->BPB_FATSz16 = FAT_GET_UINT16(&sec_bpb[BPB_FATSZ16]);
    bpb->BPB_FATSz32 = FAT_GET_UINT32(&sec_bpb[BPB_FATSZ32]);
    bpb->BPB_SecPerTrk = FAT_GET_UINT16(&sec_bpb[BPB_SECPERTRK]);
    bpb->BPB_NumHeads = FAT_GET_UINT16(&sec_bpb[BPB_NUMHEADS]);
    bpb->BPB_HiddSec = FAT_GET_UINT32(&sec_bpb[BPB_HIDDSEC]);
    bpb->BPB_TotSec32 = FAT_GET_UINT32(&sec_bpb[BPB_TOTSEC32]);
    bpb->BS_BootSign = FAT_GET_UINT16(&sec_bpb[BPB_BOOT_SIG]);
    if (bpb->BS_BootSign != 0xAA55) 
    {
        printf("It's not a FAT file system.\r\n");
        return -1;
    }

    bpb->BPB_RootEntCnt = FAT_GET_UINT16(&sec_bpb[BPB_ROOTENTCNT]);

    fatfs->fat_size = bpb->BPB_FATSz16 ? bpb->BPB_FATSz16 : bpb->BPB_FATSz32;
    fatfs->total_sectors = bpb->BPB_TotSec16 ? bpb->BPB_TotSec16 : bpb->BPB_TotSec32;

    if (bpb->BPB_BytsPerSec == 0)
    {
        printf("The FAT file system is damaged, bpb->bpb_bytspersec is 0.\r\n");
        return -1;
    }
    /* the root dir sectors always zero for FAT32 */
    fatfs->root_dir_sectors = ((bpb->BPB_RootEntCnt * FAT_DIR_ENTRY_SIZE) + (bpb->BPB_BytsPerSec - 1)) / bpb->BPB_BytsPerSec;
    fatfs->first_data_sector = bpb->BPB_RsvdSecCnt + fatfs->root_dir_sectors + bpb->BPB_NumFATs * fatfs->fat_size;

    if (bpb->BPB_SecPerClus == 0) 
    {
        printf("The FAT file system is damaged, bpb->bpb_secperclus is 0.\r\n");
        return -1;
    }
    /* determine FAT type */
    fatfs->data_clusters = (fatfs->total_sectors - fatfs->first_data_sector) / bpb->BPB_SecPerClus;
    if (fatfs->data_clusters < 4085) 
    {
        /* it should be FAT 12 */
        fatfs->fat_type = FAT_TYPE_FAT12;
        bpb->BPB_RootClus = 0;
        fatfs->fat_bits = 12;
    }
    else if (fatfs->data_clusters < 65525)
    {
        /* it should be FAT 16 */
        fatfs->fat_type = FAT_TYPE_FAT16;
        bpb->BPB_RootClus = 0;
        fatfs->fat_bits = 16;
    }
    else
    {
        /* it should be FAT 32 */
        fatfs->fat_type = FAT_TYPE_FAT32;
        fatfs->fat_bits = 28;

        /* load FAT 32 parameters */
        bpb->BPB_ExtFlags = FAT_GET_UINT16(&sec_bpb[BPB_EXTFLAGS]);
        bpb->BPB_FSVer = FAT_GET_UINT16(&sec_bpb[BPB_FSVER]);
        bpb->BPB_RootClus = FAT_GET_UINT16(&sec_bpb[BPB_ROOTCLUS]);
        bpb->BPB_FSInfo = FAT_GET_UINT16(&sec_bpb[BPB_FSINFO]);
        bpb->BPB_BkBootSec = FAT_GET_UINT16(&sec_bpb[BPB_BKBOOTSEC]);
        bpb->BS_DrvNum = sec_bpb[BS_32_DRVNUM];
    }

    bpb->BS_BootSig = sec_bpb[BS_32_BOOTSIG];
    if (bpb->BS_BootSig == 0x29) 
    {
        bpb->BS_VolID = FAT_GET_UINT32(&sec_bpb[BS_32_VOLID]);
        strncpy(bpb->BS_VolID, (char*)&sec_bpb[BS_32_VOLLAB], 11);
        strncpy(bpb->BS_FilSysType, (char*)&sec_bpb[BS_32_FILSYSTYPE], 8);
    }

    if (fatfs->fat_type & FAT_TYPE_FAT32) 
    {
        /* FAT32 */
        fatfs->root_dir_sector = first_sector_of_cluster(fatfs, bpb->BPB_RootClus);

        /* read file system info */
        result = fatdev_read(device, (device->part_start * (device->sector_size + 1)), device->sector_buff, device->sector_size);
        if (result != device->sector_size)
        {
            /* clean FAT filesystem entry */
            memset(fatfs, 0, sizeof(struct fat_fs));
            return -1;
        }

        fatfs->free_count = FAT_GET_UINT32(&sec_bpb[FAT_FSINFO_FREECNT]);
        fatfs->next_free = FAT_GET_UINT32(&sec_bpb[FAT_FSINFO_NEXTFREE]);

        /* calculate freecount if unset */
        if (fatfs->free_count == 0xffffffff) 
        {
            printf("free count is wrong.\r\n");
        }
    }
    else 
    {
        /* FAT 12/16 */
        fatfs->root_dir_sector = bpb->BPB_RsvdSecCnt + (bpb->BPB_NumFATs * bpb->BPB_FATSz16);
    }

    printf("OEM %s.\r\n", fatfs->bpb.BS_OEMName);
    printf("Bytes per sector %d.\r\n", fatfs->bpb.BPB_BytsPerSec);
    printf("Sectors per cluster %d.\r\n", fatfs->bpb.BPB_SecPerClus);
    printf("Number of reserved sector %d.\r\n", fatfs->bpb.BPB_RsvdSecCnt);
    printf("Number of FAT table %d.\r\n", fatfs->bpb.BPB_NumFATs);
    printf("Number of directories entry in root %d.\r\n", fatfs->bpb.BPB_RootEntCnt);
    printf("Number of sectors per FAT %lu.\r\n", fatfs->fat_size);
    printf("Number of sectors in root directory %d.\r\n", fatfs->root_dir_sectors);
    printf("Total sectors %lu.\r\n", fatfs->total_sectors);
    printf("Sector of root directory %lu.\r\n", fatfs->root_dir_sector);
    printf("Number of data cluster %lu.\r\n", fatfs->data_clusters);
    printf("The first data sector %lu.\r\n", fatfs->first_data_sector);
    printf("FAT type: %d.\r\n", fatfs->fat_type);
    return 0;
}

int fatck(const char* path, int sector_size)
{
    int result = -1;
    fatdev_t* device = NULL;
    device = fatdev_open(path, sector_size);
    if (device == NULL)
    {
        return result;
    }
    result = fat_root_read(device);
    if (result < 0)
    {
        printf("fat device root init failed.\r\n");
    }

    fatdev_close(device);
	return result;
}
