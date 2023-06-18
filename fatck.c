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

// FAT12/16/32 common dir field
#define DIR_NAME            (0)
#define DIR_ATTR            (11)
#define DIR_NTRES           (12)
#define DIR_CRT_TIME_TENTH  (13)
#define DIR_CRT_TIME        (14)
#define DIR_CRT_DATE        (16)
#define DIR_LST_ACC_DATE    (18)
#define DIR_FST_CLUS_HI     (20)
#define DIR_WRT_TIME        (22)
#define DIR_WRT_DATE        (24)
#define DIR_FST_CLUS_LO     (26)
#define DIR_FILE_SIZE       (28)

// file attribute
#define ATTR_READ_ONLY      (0x01)
#define ATTR_HIDDEN         (0x02)
#define ATTR_SYSTEM         (0x04)
#define ATTR_VOLUME_ID      (0x08)
#define ATTR_DIRECTORY      (0x10)
#define ATTR_ARCHIVE        (0x20)
#define ATTR_LONG_FILE_NAME (0x0F)

// Optional flags that indicates case information of the SFN.
#define SFN_BODY_LOW_CASE   (0x08)
#define SFN_EXTE_LOW_CASE   (0x10)
#define SFN_ALLS_LOW_CASE   (0x18)

#ifndef FAT_DPT_ADDRESS
#define FAT_DPT_ADDRESS     (0x1BE)
#endif
#ifndef FAT_DIR_ENTRY_SIZE
#define FAT_DIR_ENTRY_SIZE  (32)
#endif
#ifndef FAT16_INFO_SIZE
#define FAT16_INFO_SIZE     (2)
#endif
#ifndef FAT_LFN_SIZE
#define FAT_LFN_SIZE        (0xFF)
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

    uint32_t fats_sector_start;
    uint32_t fats_sector_count;
    uint32_t root_sector_start;
    uint32_t root_sector_count;
    uint32_t data_sector_start;
    uint32_t data_sector_count;
} fat_fs_t;

typedef struct fat_ck
{
    fat_dev_t* device;
    fat_fs_t fatfs;
    uint8_t* sector_buffer;
    uint32_t sector_size;
    uint32_t current_sector;
    uint32_t part_begin;
    uint16_t need_sync;
    uint16_t fat_check;
    //struct list_node node_head;
    uint8_t  test_flag;
    uint8_t  dir_deep;
    uint8_t* lfnbuf;
    uint8_t* fnbuf;
    int error;
} fat_ck_t;

typedef struct fat_dir
{
    uint8_t  DIR_Name[13];
    uint8_t  DIR_Attr;
    uint8_t  DIR_NTRes;
    uint8_t  DIR_CrtTimeTenth;
    uint16_t DIR_CrtTime;
    uint16_t DIR_CrtDate;
    uint16_t DIR_LstAccDate;
    uint16_t DIR_FstClusHI;
    uint16_t DIR_WrtTime;
    uint16_t DIR_WrtDate;
    uint16_t DIR_FstClusLO;
    uint32_t DIR_FileSize;
} fat_dir_t;

#define first_sector_of_cluster(fatfs, cluster) (((cluster)-2) * (fatfs)->bpb.BPB_SecPerClus + (fatfs)->first_data_sector)

static int fat_root_read(fat_ck_t* fc)
{
    int result = -1;
    uint8_t dpt_addr = 0;
    uint8_t* sec_bpb = NULL;
    fat_fs_t* fatfs = &fc->fatfs;
    fat_bpb_t* bpb = &fatfs->bpb;
    // get fat device sector count.
    fc->device->sector_count = fc->device->file_size / fc->device->sector_size;
    // get fat device block size.
    fc->sector_buffer = (uint8_t*)calloc(1, fc->device->sector_size * sizeof(uint8_t));
    if (fc->sector_buffer == NULL)
    {
        printf("fat check object sector buffer malloc failed.\r\n");
        return result;
    }
    result = fat_dev_read(fc->device, 0, fc->sector_buffer, fc->device->sector_size);
    if (result != fc->device->sector_size)
    {
        printf("fat root get dpt address failed.\r\n");
        return result;
    }
    // get fat device start parttion.
    dpt_addr = fc->sector_buffer[FAT_DPT_ADDRESS];
    if ((dpt_addr != 0x80) && (dpt_addr != 0x00))
    {
        fc->device->part_start = 0;
    }
    else
    {
        fc->device->part_start = FAT_GET_UINT32(&fc->sector_buffer[FAT_DPT_ADDRESS] + 8);
    }
    result = fat_dev_read(fc->device, (fc->device->part_start * fc->device->sector_size), fc->sector_buffer, fc->device->sector_size);
    if (result != fc->device->sector_size)
    {
        printf("fat root read start parttion failed.\r\n");
        return result;
    }
    // get fat device bpb sector.
    sec_bpb = fc->sector_buffer;
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
        strncpy(bpb->BS_VolLab, (char*)&sec_bpb[BS_32_VOLLAB], 11);
        strncpy(bpb->BS_FilSysType, (char*)&sec_bpb[BS_32_FILSYSTYPE], 8);
    }

    if (fatfs->fat_type & FAT_TYPE_FAT32) 
    {
        /* FAT32 */
        fatfs->root_dir_sector = first_sector_of_cluster(fatfs, bpb->BPB_RootClus);

        /* read file system info */
        result = fat_dev_read(fc->device, ((fc->device->part_start + 1) * fc->device->sector_size), fc->sector_buffer, fc->device->sector_size);
        if (result != fc->device->sector_size)
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

static int fat_fats_check(fat_ck_t* fc, uint32_t start, uint32_t end)
{
    int result = -1;
    uint8_t  fat_info[FAT16_INFO_SIZE] = { 0x00 };
    uint16_t value = 0;
    uint16_t count = 0;
    uint32_t index_addr = start + (2 * sizeof(fat_info));
    uint32_t start_addr = 0;
    
    while (true)
    {
        // read finished, break loop.
        if (index_addr >= end)
        {
            break;
        }
        result = fat_dev_read(fc->device, index_addr, fat_info, sizeof(fat_info));
        if (result == sizeof(fat_info))
        {
            value = FAT_GET_UINT16(&fat_info[0]);
            if (value == 0x0000)
            {
                printf("Addr [0x%08X - 0x%08X]: %-4d Clusters Free.\r\n", index_addr, index_addr, 1);
                break;
            }
            else if (value == 0x0001)
            {
                printf("Addr [0x%08X - 0x%08X]: %-4d Clusters Reserved.\r\n", index_addr, index_addr, 1);
            }
            else if ((value > 0x0002) && (value <= 0xFFF6))
            {
                count = count + 1;
                start_addr = (start_addr == 0) ? index_addr : start_addr;
            }
            else if (value == 0xFFF7)
            {
                printf("Addr [0x%08X - 0x%08X]: %-4d Clusters Bad.\r\n", index_addr, index_addr, 1);
            }
            else if ((value >= 0xFFF8) && (value <= 0xFFFF))
            {
                count = (start_addr == 0) ? 1 : count + 1;
                start_addr = (start_addr == 0) ? index_addr : start_addr;
                printf("Addr [0x%08X - 0x%08X]: %-4d Clusters Use.\r\n", start_addr, index_addr, count);
                start_addr = 0;
                count = 0;
            }
            index_addr = index_addr + sizeof(fat_info);
        }
        else
        {
            result = 0;
            break;
        }
    }
    return result;
}

static int fat_fats_count(fat_ck_t* fc, uint32_t index)
{
    int result = -1;
    uint8_t  fat_info[FAT16_INFO_SIZE] = { 0x00 };
    uint32_t fat_addr = (fc->fatfs.fats_sector_start * fc->device->sector_size) + (index * sizeof(fat_info));
    uint16_t value = 0;
    uint16_t count = 0;

    while (true)
    {
        result = fat_dev_read(fc->device, fat_addr, fat_info, sizeof(fat_info));
        if (result == sizeof(fat_info))
        {
            value = FAT_GET_UINT16(&fat_info[0]);
            if (value == 0x0000)
            {
                count = 0;
                break;
            }
            else if (value == 0x0001)
            {
                count = 0;
                break;
            }
            else if ((value > 0x0002) && (value <= 0xFFF6))
            {
                count = count + 1;
            }
            else if (value == 0xFFF7)
            {
                fat_addr = fat_addr + sizeof(fat_info);
                continue;
            }
            else if ((value >= 0xFFF8) && (value <= 0xFFFF))
            {
                count = count + 1;
                break;
            }
            fat_addr = fat_addr + sizeof(fat_info);
        }
        else
        {
            return count;
        }
    }
    return count;
}

static int fat_lfn_read(fat_ck_t* fc, uint32_t start, uint32_t count, uint8_t *name, uint8_t size)
{
    int result = -1;
    uint8_t dir_info[FAT_DIR_ENTRY_SIZE] = { 0x00 };
    uint32_t sfn_addr = start;
    int index = 0, i = 0, j = 0;
    // read long name info.
    for (index = 1; index < count + 1; index++)
    {
        result = fat_dev_read(fc->device, (sfn_addr - (sizeof(dir_info) * index)), dir_info, sizeof(dir_info));
        if ((result == sizeof(dir_info)) && (dir_info[0] != '\0'))
        {
            for (i = 0x01; (i < 0x0B) && (j < size); i = i + 2, j = j + 1)
            {
                if (dir_info[i] != 0xFF)
                {
                    name[j] = dir_info[i];
                }
            }

            for (i = 0x0E; (i < 0x1A) && (j < size); i = i + 2, j = j + 1)
            {
                if (dir_info[i] != 0xFF)
                {
                    name[j] = dir_info[i];
                }
            }

            for (i = 0x1C; (i < 0x20) && (j < size); i = i + 2, j = j + 1)
            {
                if (dir_info[i] != 0xFF)
                {
                    name[j] = dir_info[i];
                }
            }
        }
    }
    // set name end.
    if (j + 1 < size)
    {
        name[j + 1] = '\0';
    }
    else
    {
        name[size - 1] = '\0';
    }
    return 0;
}

static int fat_sfn_read(char name[13], uint8_t attr)
{
    size_t index = 0;
    size_t useds = 0;
    size_t start = 0;
    size_t limit = 0;
    char temp[13] = { 0 };

    switch (attr)
    {
    case SFN_BODY_LOW_CASE:
        start = 0x00; limit = 0x08;
        break;
    case SFN_EXTE_LOW_CASE:
        start = 0x08; limit = 0x0B;
        break;
    case SFN_ALLS_LOW_CASE:
        start = 0x00; limit = 0x0B;
        break;
    default:
        start = 0x00; limit = 0x00;
        break;
    }
    memset(temp, 0, sizeof(temp));
    // uper char transfer to lower char.
    for (index = 0; (index < 11) && (useds < 13); index++)
    {
        if ((index >= start) && (index < limit) && (name[index] >= 'A') && (name[index] <= 'Z'))
        {
            name[index] = tolower(name[index]);
        }
        if ((name[index] >= 'A') && (name[index] <= 'z'))
        {
            temp[useds] = name[index];
            useds = useds + 1;
        }
        if ((index == 0x07) && (name[index] == ' '))
        {
            temp[useds] = '.';
            useds = useds + 1;
        }
    }
    if (useds < 13)
    {
        temp[useds] = '\0';
    }
    else
    {
        temp[12] = '\0';
    }
    memcpy(name, temp, sizeof(temp));
    return 0;
}

static int fat_dirs_check(fat_ck_t* fc, uint32_t start, uint32_t end)
{
    int result = -1;
    uint8_t dir_info[FAT_DIR_ENTRY_SIZE] = { 0x00 };
    fat_dir_t dir = { 0 };
    uint8_t lfn_cnt = 0;
    uint8_t lfn_buf[FAT_LFN_SIZE] = { 0x00 };
    
    while (true)
    {
        // read finished, break loop.
        if (start >= end)
        {
            break;
        }
        result = fat_dev_read(fc->device, start, dir_info, sizeof(dir_info));
        if ((result == sizeof(dir_info)) && (dir_info[0] != '\0'))
        {
            if (((dir_info[0] == 0x2E) && (dir_info[1] == 0x20)) || \
                ((dir_info[0] == 0x2E) && (dir_info[1] == 0x2E) && (dir_info[2] == 0x20)))
            {
                start = start + sizeof(dir_info);
                continue;
            }
            // this is short file name or other files.
            if (dir_info[DIR_ATTR] == ATTR_READ_ONLY || \
                dir_info[DIR_ATTR] == ATTR_HIDDEN    || \
                dir_info[DIR_ATTR] == ATTR_SYSTEM    || \
                dir_info[DIR_ATTR] == ATTR_VOLUME_ID || \
                dir_info[DIR_ATTR] == ATTR_DIRECTORY || \
                dir_info[DIR_ATTR] == ATTR_ARCHIVE)
            {
                printf("\r\n========== Dir info ==========\r\n");
                memset(&dir, 0, sizeof(fat_dir_t));
                strncpy(dir.DIR_Name, dir_info, sizeof(dir.DIR_Name) - 2);
                dir.DIR_Attr = dir_info[DIR_ATTR];
                dir.DIR_NTRes = dir_info[DIR_NTRES];
                dir.DIR_CrtTimeTenth = dir_info[DIR_CRT_TIME_TENTH];
                dir.DIR_CrtTime = FAT_GET_UINT16(&dir_info[DIR_CRT_TIME]);
                dir.DIR_CrtDate = FAT_GET_UINT16(&dir_info[DIR_CRT_DATE]);
                dir.DIR_LstAccDate = FAT_GET_UINT16(&dir_info[DIR_LST_ACC_DATE]);
                dir.DIR_FstClusHI = FAT_GET_UINT16(&dir_info[DIR_FST_CLUS_HI]);
                dir.DIR_WrtTime = FAT_GET_UINT16(&dir_info[DIR_WRT_TIME]);
                dir.DIR_WrtDate = FAT_GET_UINT16(&dir_info[DIR_WRT_DATE]);
                dir.DIR_FstClusLO = FAT_GET_UINT16(&dir_info[DIR_FST_CLUS_LO]);
                dir.DIR_FileSize = FAT_GET_UINT32(&dir_info[DIR_FILE_SIZE]);
                // long file name
                if (lfn_cnt > 0)
                {
                    fat_lfn_read(fc, start, lfn_cnt, &lfn_buf, FAT_LFN_SIZE);
                    printf("DIR_Name         : %s \r\n", lfn_buf);
                    lfn_cnt = 0;
                }
                // short file name.
                else
                {
                    fat_sfn_read(dir.DIR_Name, dir.DIR_NTRes);
                    printf("DIR_Name         : %s \r\n", dir.DIR_Name);
                }
                // other file attr.
                printf("DIR_Attr         : 0x%02X \r\n", dir.DIR_Attr);
                printf("DIR_NTRes        : 0x%02X \r\n", dir.DIR_NTRes);
                printf("DIR_CrtTimeTenth : %d \r\n", dir.DIR_CrtTimeTenth);
                printf("DIR_CrtTime      : %d \r\n", dir.DIR_CrtTime);
                printf("DIR_CrtDate      : %d \r\n", dir.DIR_CrtDate);
                printf("DIR_LstAccDate   : %d \r\n", dir.DIR_LstAccDate);
                printf("DIR_FstClusHI    : %d \r\n", dir.DIR_FstClusHI);
                printf("DIR_WrtTime      : %d \r\n", dir.DIR_WrtTime);
                printf("DIR_WrtDate      : %d \r\n", dir.DIR_WrtDate);
                printf("DIR_FstClusLO    : %d \r\n", dir.DIR_FstClusLO);
                printf("DIR_FileSize     : %d \r\n", dir.DIR_FileSize);

                if ((dir.DIR_FstClusLO > 0) && (dir.DIR_FileSize == 0))
                {
                    uint16_t count = fat_fats_count(fc, dir.DIR_FstClusLO);
                    uint32_t addrs = (fc->fatfs.root_sector_start + 2 + dir.DIR_FstClusLO) * fc->device->sector_size;
                    uint32_t stops = addrs + count * fc->device->sector_size;
                    fat_dirs_check(fc, addrs, stops);
                }
            }
            // this is long file name.
            else if (dir_info[DIR_ATTR] == ATTR_LONG_FILE_NAME)
            {
                //printf("Long file name info.\r\n");
                lfn_cnt = lfn_cnt + 1;
            }
            else
            {
                printf("Unknown file name info.\r\n");
                break;
            }
            // read next info.
            start = start + sizeof(dir_info);
        }
        else
        {
            result = 0;
            break;
        }
    }
    return result;
}

static int fat_root_check(fat_ck_t* fc)
{
    int result = -1;

    fat_bpb_t* bpb = &fc->fatfs.bpb;
    fc->fatfs.fats_sector_start = fc->device->part_start + bpb->BPB_RsvdSecCnt;
    fc->fatfs.fats_sector_count = bpb->BPB_FATSz16 * bpb->BPB_NumFATs;
    fc->fatfs.root_sector_start = fc->fatfs.fats_sector_start + fc->fatfs.fats_sector_count;
    fc->fatfs.root_sector_count = (32 * bpb->BPB_RootEntCnt + bpb->BPB_BytsPerSec - 1) / bpb->BPB_BytsPerSec;
    fc->fatfs.data_sector_start = fc->fatfs.root_sector_start + fc->fatfs.root_sector_count;
    fc->fatfs.data_sector_count = bpb->BPB_TotSec16 - fc->fatfs.data_sector_start;

    printf("\r\n");
    printf("fats_sector_start %d.\r\n", fc->fatfs.fats_sector_start);
    printf("fats_sector_count %d.\r\n", fc->fatfs.fats_sector_count);
    printf("root_sector_start %d.\r\n", fc->fatfs.root_sector_start);
    printf("root_sector_count %d.\r\n", fc->fatfs.root_sector_count);
    printf("data_sector_start %d.\r\n", fc->fatfs.data_sector_start);
    printf("data_sector_count %d.\r\n", fc->fatfs.data_sector_count);
    
    // process fat table
    uint32_t fats_start = (fc->fatfs.fats_sector_start * fc->device->sector_size);
    uint32_t fats_end = (fc->fatfs.fats_sector_start + fc->fatfs.fats_sector_count) * fc->device->sector_size;
    fat_fats_check(fc, fats_start, fats_end);

    // process fat root directory
    uint32_t root_start = (fc->fatfs.root_sector_start * fc->device->sector_size);
    uint32_t root_end = (fc->fatfs.root_sector_start + fc->fatfs.root_sector_count) * fc->device->sector_size;
    fat_dirs_check(fc, root_start, root_end);

    // process fat data
    uint32_t root_sub_start = ((fc->fatfs.root_sector_start + 4) * fc->device->sector_size);
    uint32_t root_sub_end = (fc->fatfs.root_sector_start + 5) * fc->device->sector_size;
    fat_dirs_check(fc, root_sub_start, root_sub_end);

    return 0;
}

int fatck(const char* path, int sector_size)
{
    int result = -1;
    fat_ck_t *fc = (fat_ck_t*)calloc(1, sizeof(fat_ck_t));
    if (fc == NULL)
    {
        printf("fat check object create failed.\r\n");
        return result;
    }
    fc->device = fat_dev_open(path, sector_size);
    if (fc->device == NULL)
    {
        printf("fat device object open failed.\r\n");
        return result;
    }
    result = fat_root_read(fc);
    if (result < 0)
    {
        printf("fat device root read failed.\r\n");
    }
    result = fat_root_check(fc);
    if (result < 0)
    {
        printf("fat device root check failed.\r\n");
    }
    fat_dev_close(fc->device);
    if (fc->device != NULL)
    {
        free(fc->device);
    }
    if (fc->sector_buffer != NULL)
    {
        free(fc->sector_buffer);
    }
    free(fc);
    return result;
}
