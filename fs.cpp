#include "fs.h"

using namespace std;

struct SuperBlock
{
    bitset<MAX_INFO_BLOCK> info;
    bitset<MAX_DATA_BLOCK> data;
    int findEmptyInfo()
    {
        for (int i = 0; i < MAX_INFO_BLOCK; i++)
            if (info[i])
                return i;
        return -1;
    }
    int findEmptyData()
    {
        for (int i = 0; i < MAX_DATA_BLOCK; i++)
            if (data[i])
                return i;
        return -1;
    }
};
SuperBlock super_block;

class InfoBlock
{
public:
    int *fat;
    int count;

    InfoBlock()
    {
        fat = new int[MAX_ENTRY];
        count = 0;
        for (int i = 0; i < MAX_ENTRY; i++)
        {
            fat[i] = -1;
        }
    }

    void fat_append(int index)
    {
        for (int i = 0; i < MAX_ENTRY; i++)
        {
            if (fat[i] == -1)
            {
                fat[i] = index;
                super_block.data.flip(index);
                count++;
                return;
            }
        }
    }

    void fat_delete()
    {
        for (int i = 0; i < MAX_ENTRY; i++)
            if (fat[i] == -1)
            {
                return;
            }
            else
            {
                fat[i] = -1;
                super_block.data.flip(fat[i]);
                count--;
            }
    }

    void fat_write(int index)
    {
        index = 2 + index * 4;
        char buf[BLOCK_SIZE];
        for (int i = 0; i < 4; i++)
        {
            memset(buf, 0, BLOCK_SIZE);
            memcpy(buf, fat + i * 1024, BLOCK_SIZE);
            block_write(index + i, buf);
        }
    }

    ~InfoBlock() { 
        delete[] fat; 
    }
};

InfoBlock *getInfoBlock(int index)
{
    index = 2 + index * 4;
    InfoBlock *info = new InfoBlock();
    for (int i = 0; i < 4; i++)
    {
        char buf[BLOCK_SIZE];
        int fat[1024];
        block_read(index + i, buf);
        memcpy(&fat, &buf, BLOCK_SIZE);
        for (int n = 0; n < 1024; n++)
        {
            info->fat[i * 1024 + n] = fat[n];
        }
    }
    return info;
}

struct Directory
{
    char name[15];
    int size = 0;
    int index = -1;
    bool empty = true;
};

Directory directory_list[MAX_FILE];

int find_dir_index(char *name)
{
    for (int i = 0; i < MAX_FILE; i++)
    {
        if (!directory_list[i].empty && strcmp(name, directory_list[i].name) == 0)
        {
            return i;
        }
    }
    return -1;
}

int find_empty_dir()
{
    for (int i = 0; i < MAX_FILE; i++)
    {
        if (directory_list[i].empty)
        {
            return i;
        }
    }
    return -1;
}

map<int, char *> open_fd_name;
map<int, int> fd_offset;
int fd = 0;

int make_fs(char *disk_name)
{
    if (make_disk(disk_name) == -1 || open_disk(disk_name) == -1)
    {
        return -1;
    }
    super_block.info.set();
    super_block.data.set();
    for (int i = 0; i < MAX_FILE; i++)
    {
        Directory directory;
        directory_list[i] = directory;
    }
    char buf[BLOCK_SIZE];
    memset(buf, 0, BLOCK_SIZE);
    memcpy(&buf, &super_block, sizeof(super_block));
    block_write(0, buf);
    memset(buf, 0, BLOCK_SIZE);
    memcpy(&buf, &directory_list, sizeof(directory_list));
    block_write(1, buf);
    return close_disk();
}

int mount_fs(char *disk_name)
{
    if (open_disk(disk_name) == -1)
    {
        return -1;
    }

    char buf[BLOCK_SIZE];
    memset(buf, 0, BLOCK_SIZE);
    block_read(0, buf);
    memcpy(&super_block, buf, sizeof(super_block));
    memset(buf, 0, BLOCK_SIZE);
    block_read(1, buf);
    memcpy(&directory_list, buf, sizeof(directory_list));
    return 0;
}

int umount_fs(char *disk_name)
{
    char buf[BLOCK_SIZE];
    memset(buf, 0, BLOCK_SIZE);
    memcpy(&buf, &super_block, sizeof(super_block));
    block_write(0, buf);
    memset(buf, 0, BLOCK_SIZE);
    memcpy(&buf, &directory_list, sizeof(directory_list));
    block_write(1, buf);
    return close_disk();
}

int fs_open(char *name)
{
    if (strlen(name) > 15)
    {
        return -1;
    }
    if (open_fd_name.size() == 32)
    {
        return -1;
    }
    if (find_dir_index(name) == -1)
    {
        return -1;
    }
    fd++;
    open_fd_name[fd] = name;
    fd_offset[fd] = 0;
    return fd;
}

int fs_close(int fildes)
{
    if (open_fd_name.find(fildes) == open_fd_name.end())
    {
        return -1;
    }
    fd_offset.erase(fildes);
    open_fd_name.erase(fildes);
    return 0;
}

int fs_create(char *name)
{
    int directory_index;
    if (strlen(name) > 15)
    {
        return -1;
    }
    if (open_fd_name.size() == 32)
    {
        return -1;
    }
    if (find_dir_index(name) != -1)
    {
        return -1;
    }
    if ((directory_index = find_empty_dir()) == -1)
    {
        return -1;
    }
    Directory directory;
    int info_index = super_block.findEmptyInfo();
    strncpy(directory.name, name, sizeof(name));
    directory.size = 0;
    directory.index = info_index;
    directory.empty = false;
    directory_list[directory_index] = directory;
    super_block.info.flip(info_index);
    InfoBlock *info = new InfoBlock();
    info->fat_write(info_index);
    return 0;
}

int fs_delete(char *name)
{
    for (auto iter = open_fd_name.begin(); iter != open_fd_name.end(); iter++)
    {
        if (iter->second == name)
        {
            return -1;
        }
    }
    int directory_index = find_dir_index(name);
    if (directory_index == -1)
    {
        return -1;
    }

    Directory directory = directory_list[directory_index];
    super_block.info.flip(directory.index); 
    InfoBlock *info = getInfoBlock(directory.index);
    info->fat_delete();
    info->fat_write(directory.index);

    for (int i = 0; i < MAX_FILE; i++)
    {
        if (strcmp(directory.name, directory_list[i].name) == 0)
        {
            directory_list[i].empty = true;
            break;
        }
    }
    return 0;
}

int fs_read(int fildes, void *buf, size_t nbyte)
{
    memset(buf, 0, nbyte);
    if (open_fd_name.find(fildes) == open_fd_name.end())
    {
        return -1;
    }
    Directory directory = directory_list[find_dir_index(open_fd_name[fildes])];
    InfoBlock *info = getInfoBlock(directory.index);
    int offset = fd_offset[fildes];
    if (offset == directory.size)
    {
        return 0;
    }
    int bytes;
    if (nbyte + offset > directory.size)
    {
        bytes = directory.size - offset;
    }
    else
    {
        bytes = nbyte;
    }
    int max = directory.size / BLOCK_SIZE + (directory.size % BLOCK_SIZE);
    char temp[max * BLOCK_SIZE];
    int current = 0;
    for (int i = 0; i < max; i++)
    {
        block_read(info->fat[i] + 4096, temp + current);
        current += BLOCK_SIZE;
    }
    memcpy(buf, temp + offset, bytes);
    fd_offset[fildes] += bytes;
    return bytes;
}

int fs_write(int fildes, void *buf, size_t nbyte)
{
    if (open_fd_name.find(fildes) == open_fd_name.end())
    {
        return -1;
    }
    int directory_index = find_dir_index(open_fd_name[fildes]);
    Directory directory = directory_list[directory_index];
    int offset = fd_offset[fildes];
    fd_offset[fildes] = offset + nbyte;
    char temp[BLOCK_SIZE];
    int current = 0;
    int bytes = nbyte;
    InfoBlock *info = getInfoBlock(directory.index);

    for (int i = offset / BLOCK_SIZE; i < info->count; i++)
    {
        memset(temp, 0, BLOCK_SIZE);
        block_read(info->fat[i] + 4096, temp);
        int n = (BLOCK_SIZE - offset < nbyte) ? BLOCK_SIZE - offset : nbyte;
        memcpy(temp + offset, (char *)buf + current, n);
        current += n;
        block_write(info->fat[i] + 4096, temp);
        offset = 0;
    }

    int data_index;
    for (data_index = super_block.findEmptyData(); current < nbyte && data_index > -1; data_index = super_block.findEmptyData())
    {
        memcpy(temp, (char *)buf + current, (bytes < BLOCK_SIZE) ? bytes : BLOCK_SIZE);
        block_write(data_index + 4096, temp);
        current += BLOCK_SIZE;
        bytes -= BLOCK_SIZE;
        info->fat_append(data_index);
        memset(temp, 0, BLOCK_SIZE);
    }
    directory_list[directory_index].size = (fd_offset[fildes] < directory.size) ? directory.size : fd_offset[fildes];
    info->fat_write(directory.index);
    return (data_index == -1 && current < nbyte) ? current : nbyte;
}

int fs_get_filesize(int fildes)
{
    if (find_dir_index(open_fd_name[fildes]) == -1)
    {
        return -1;
    }
    else
    {
        return directory_list[find_dir_index(open_fd_name[fildes])].size;
    }
}

int fs_lseek(int fildes, off_t offset)
{
    if (open_fd_name.find(fildes) == open_fd_name.end() || offset < 0 || offset > fs_get_filesize(fildes))
    {
        return -1;
    }
    fd_offset[fildes] = offset;
    return 0;
}

int fs_truncate(int fildes, off_t length)
{
    if (find_dir_index(open_fd_name[fildes]) == -1)
    {
        return -1;
    }
    if (open_fd_name.find(fildes) == open_fd_name.end())
    {
        return -1;
    }
    Directory directory = directory_list[find_dir_index(open_fd_name[fildes])];
    if (directory.size < length)
    {
        return -1;
    }
    if (fd_offset[fildes] > length)
    {
        fd_offset[fildes] = length;
    }
    InfoBlock *info = getInfoBlock(directory.index);
    char buf[BLOCK_SIZE];
    int start = length / BLOCK_SIZE;
    int end = directory.size / BLOCK_SIZE + (directory.size % BLOCK_SIZE != 0);
    if (length % BLOCK_SIZE)
    {
        char read_buf[BLOCK_SIZE];
        char write_buf[BLOCK_SIZE];
        block_read(info->fat[start] + 4096, read_buf);
        for (int i = 0; i < length % BLOCK_SIZE; i++)
        {
            write_buf[i] = read_buf[i];
        }
        block_write(info->fat[start++] + 4096, write_buf);
    }
    for (int i = start; i < end; i++)
    {
        block_write(info->fat[i] + 4096, buf);
        super_block.data.flip(info->fat[i]);
    }
    directory_list[find_dir_index(open_fd_name[fildes])].size = length;
    return 0;
}