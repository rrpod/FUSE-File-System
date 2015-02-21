#define FUSE_USE_VERSION 26

#include <iostream>
#include <map>
#include <string>
#include <list>
#include <fuse.h>
#include <errno.h>
#include <libgen.h>
#include <fcntl.h>
#include <string.h>
#include <malloc.h>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <time.h>
using namespace std;


#define MAP_OVERHEAD 20
int gInodeCounter =1;
unsigned int TotalMemory = 0;
unsigned int UsedMem = 0;
char DUMMY[255]={0};
struct st_node
{
    int inode;
    char *pcDataBlock;
    char type;
    char *name;
    size_t size;
    uid_t uid;
    gid_t gid;
    mode_t mode;
    time_t ctime;
    time_t atime;
    time_t mtime;
    list<int> LInodes;    
}st_node;

map <string,int > M_Inode;
map <int, struct st_node *> M_struct;

static int custom_getattr(const char *path,struct stat *stbuf)
{
    int result = -ENOENT;
    memset(stbuf,0,sizeof(stbuf));
    string S_path(path);
    if(M_Inode.count(S_path) == 0)
    {
        return result;
    }
    int inode = M_Inode[S_path];
    struct st_node *pst = M_struct[inode];
    
    if(pst->type == 'F')
        stbuf->st_mode =S_IFREG |pst->mode;
    else
         stbuf->st_mode =S_IFDIR |pst->mode;
    stbuf->st_uid = pst->uid;
    stbuf->st_gid = pst->gid;
    stbuf->st_size = pst->size;
    stbuf->st_atim.tv_sec = pst->atime;
    stbuf->st_ctim.tv_sec = pst->ctime;
    stbuf->st_mtim.tv_sec = pst->mtime;
    return 0;
}

static int custom_readdir (const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
 
    int result = -ENOENT;
    string S_path(path);
    if(M_Inode.count(S_path) == 0)
    {
        return result;
    }
    int inode = M_Inode[S_path];
    struct st_node *pst = M_struct[inode];
    
    filler (buf, ".", NULL, 0);
    filler (buf, "..", NULL, 0);
    
    int i;
    for(list<int>::iterator it= pst->LInodes.begin();it !=pst->LInodes.end();it++)
    {
       struct st_node *pst1 = M_struct[*it];
       filler(buf,pst1->name,NULL,0);
    }
    return 0;
}

static int custom_read (const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *f)
{
    int result = -ENOENT;
    string S_path(path);
    if(M_Inode.count(S_path) == 0)
    {
        return result;
    }
    int inode = M_Inode[S_path];
    struct st_node *pst = M_struct[inode];

    if(offset > pst->size)
        size =0;
    if(offset + size > pst->size )
        size = pst->size - offset;
    if(size > 0)
        memcpy(buf,pst->pcDataBlock,size);
    time(&pst->atime);
    if(UsedMem > TotalMemory)
    {
        cout<<"how";
    }
    return size;
}

static int custom_write (const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *f)
{
    int result = -ENOENT;
    string S_path(path);
    if(M_Inode.count(S_path) == 0)
    {
        return result;
    }
    int tempMem = size;
    if(tempMem + UsedMem > TotalMemory)
    {
        return -ENOMEM;
    }
    else
        UsedMem+= tempMem;
    if(UsedMem > TotalMemory)
    {
        cout<<"how";
    }
    int inode = M_Inode[S_path];
    struct st_node *pst = M_struct[inode];

    //if(offset + size  > pst->size)
    if(pst->size == 0)
    {
        pst->pcDataBlock =(char *) malloc(size);
        memcpy(pst->pcDataBlock, buf , size);
        pst->size = size;
    }
    else
    {
        pst->pcDataBlock = (char *)realloc(pst->pcDataBlock,offset +size);
        pst->size = offset + size;
        memcpy(pst->pcDataBlock+offset,buf,size);
    }
       time(&pst->mtime);
    return size;
}

static int custom_create (const char *path, mode_t mode, struct fuse_file_info *f)
{
    char *pathd = strdup (path);
    char *pathb = strdup (path);
    char *dir = dirname (pathd);
    char *base = basename (pathb);   
    
    string S_Path(path);
    string S_dir(dir);
    string S_base(base);
    
   
    int tempMem = sizeof(struct st_node);
    tempMem += strlen(path);
    tempMem += MAP_OVERHEAD;
    if(tempMem + UsedMem > TotalMemory)
    {
        return -ENOMEM;
    }
    else
        UsedMem+= tempMem;
    if(UsedMem > TotalMemory)
    {
        cout<<"how";
    }
    struct st_node *pst = new struct st_node;
    pst->inode = gInodeCounter;
    pst->type = 'F';
    pst->size = 0;
    pst->gid = fuse_get_context()->gid;
    pst->uid = fuse_get_context()->uid;
    pst->name = strdup(base);
    pst->mode = mode;
    pst->pcDataBlock = NULL;
    
    M_Inode[S_Path]= gInodeCounter ;
    M_struct[gInodeCounter] = pst;
    
    int newIn = M_Inode[S_dir];
    struct st_node *pst_parent = M_struct[newIn];
    pst_parent->LInodes.push_back(gInodeCounter);
    gInodeCounter++;
    time(&pst->ctime);
    return 0;
}

static int custom_mkdir (const char *path, mode_t mode)
{
    int result = -EACCES;
    char *pathd = strdup (path);
    char *pathb = strdup (path);
    char *dir = dirname (pathd);
    char *base = basename (pathb);
    
    string DIR(dir);
    string S_path(path);
    if(M_Inode.count(S_path) == 1)
        return -EACCES;
    if(M_Inode.count(DIR)!=1)
        return -ENOENT;
    
    int tempMem = sizeof(struct st_node);
    tempMem += strlen(path);
    tempMem += MAP_OVERHEAD;
    if(tempMem + UsedMem > TotalMemory)
    {
        return -ENOMEM;
    }
    else
        UsedMem+= tempMem;
    
    struct st_node *pst = new struct st_node;
    pst->inode = gInodeCounter;
    pst->type = 'D';
    pst->size = 0;
    pst->gid = fuse_get_context()->gid;
    pst->uid = fuse_get_context()->uid;
    pst->name = strdup(base);
    pst->mode = mode;
    pst->pcDataBlock = NULL;
    
    M_Inode[S_path]=  gInodeCounter;
    M_struct[gInodeCounter] = pst;
    
    int newIn = M_Inode[DIR];
    struct st_node *pst_parent = M_struct[newIn];
    pst_parent->LInodes.push_back(gInodeCounter);
    gInodeCounter++;
    time(&pst->ctime);
    if(UsedMem > TotalMemory)
    {
        cout<<"how";
    }
    return 0;
}

static int custom_unlink (const char *path)
{
    int result = -ENOENT;
    string S_path(path);
    if(M_Inode.count(S_path) == 0)
    {
        return result;
    }
    char *pathd = strdup (path);
    char *pathb = strdup (path);
    char *dir = dirname (pathd);
    char *base = basename (pathb);
    
    string DIR(dir);
    
    int tempMem = sizeof(struct st_node);
    tempMem += strlen(path);
    
    int inode = M_Inode[S_path];
    struct st_node *pst = M_struct[inode];
    
    if(pst->type == 'D')
    {
        if(pst->LInodes.size()> 0)
            return -ENOTEMPTY;
    }
    if(pst->type == 'F')
    {
        if(pst->pcDataBlock)
        {
            free(pst->pcDataBlock);
            tempMem += pst->size;
        }
    }
    if(pst->name)
        free(pst->name);

    delete pst;
    
    int newIn = M_Inode[DIR];
    struct st_node *pst_parent = M_struct[newIn];
    
    //struct st_node *pst_parent = M_struct[DIR];
    pst_parent->LInodes.remove(inode);
    
    M_Inode.erase(S_path);
    M_struct.erase(inode);

    tempMem-= MAP_OVERHEAD;

    if(UsedMem - tempMem < 0)
        UsedMem= 0;
    else
        UsedMem -= tempMem;
    return 0;
   
    
}
static int custom_rmdir (const char *path)
{
     return custom_unlink (path);
}

static int custom_truncate (const char *path, off_t size)
{
    int result = -ENOENT;
    string S_path(path);
    if(M_Inode.count(S_path) == 0)
    {
        return result;
    }
    
    int inode = M_Inode[S_path];
    struct st_node *pst = M_struct[inode];
    int tempMem =0;
    
    if(size == 0)
    {
        if(pst->pcDataBlock)
            free(pst->pcDataBlock);
        pst->pcDataBlock = NULL;
        tempMem = pst->size;
        pst->size = 0;
        
    }
    else
    {
        tempMem = pst->size;
        UsedMem-= tempMem;
        
        pst->pcDataBlock = (char *)realloc(pst->pcDataBlock ,size);
        pst->size = size;
        UsedMem += size;
    }
    return 0;
}

static int custom_open (const char *path, struct fuse_file_info *f)
{
    
    int result = -ENOENT;
    string S_path(path);
    if(M_Inode.count(S_path) == 0)
    {
        return result;
    }
    
    return 0;
}

static int custom_release (const char *path, struct fuse_file_info *f)
{
    
    int result = -ENOENT;
    string S_path(path);
    if(M_Inode.count(S_path) == 0)
    {
        return result;
    }
    return 0;
}


int readRam(char *path)
{
    fstream fs;
    string a;
    int b;
	cout<<"Path "<<path<<"\n";
    //fs.open("/home/rohitrp/NetBeansProjects/FUSE/ramdisk.store",\fstream::in|fstream::out|fstream::binary);
    ifstream infile(path);
    if(infile.is_open())
    {
        std::streampos fsize = 0;
        fsize = infile.tellg();
        infile.seekg( 0, std::ios::end );
        fsize = infile.tellg() - fsize;
        infile.seekg(0,std::ios::beg);
        if(fsize == 0)
        {
            //Nothing to read
            return 0;
        }
        while(infile >> a >>b)
        {
            if(a == "INODE_MAP_OVER")
                break;
            M_Inode[a] = b;
        }
        cout<<M_Inode.size()<<"\n";
        for(map<string,int>::iterator it= M_Inode.begin(); it!=M_Inode.end();it++)
        {
            cout<<it->first<<" "<<it->second<<"\n";
        }
        string line;
        long int c;
        
        string pcData;
        string temp;
        int count =1;
        while(getline(infile,line))
        {
            struct st_node *obj = new struct st_node;
            istringstream iss(line);
            if(count ==1 )
            {
                count =2;
                continue;
            }
            else
            {
                std::size_t found = line.find("STRUCT_MAP_OVER");
                if (found!=std::string::npos)
                {
                    string a;
                    iss>>a;
                    iss>>UsedMem;
                    iss>>gInodeCounter;
                    if(UsedMem > TotalMemory)
                    {
                        cout<<"Too less memory allocated to load old RAMDISK..Try Loading with higher size \n";
                        exit(0);
                    }
                    else
                        break;
                }
                else
                    iss>>b;
            }
            iss>>obj->atime;
            iss>>obj->ctime;
            iss>>obj->gid;
            iss>>obj->mode;
            iss>>obj->mtime;
            iss>>pcData;
            if(pcData == "<NULL>")
                obj->pcDataBlock = NULL;
            else if(pcData == "<FILE_DATA_BLOCK>")
            {
                pcData.clear();
                int flag=0;
                string bin;
                while(getline(infile,line))
                {
                    if(line=="<FILE_DATA_BLOCK>")
                        break;
                    if(flag == 0)
                        flag =1;
                    else
                        pcData.append("\n");
                    pcData.append(line);
                }
                obj->pcDataBlock =(char *)malloc(pcData.size()+2);
                memset(obj->pcDataBlock,0,pcData.size()+2);
                memcpy(obj->pcDataBlock,pcData.data(),pcData.size());
		memcpy(obj->pcDataBlock + pcData.size(),"\n",strlen("\n"));                
                pcData.clear();
                getline(infile,line);
                iss.clear();
                iss.str(line);

            }        
            iss>>pcData;
            if(pcData == "<NULL>")
                obj->name = NULL;
            else
                obj->name = strdup(pcData.c_str());

            //iss>>pcData;
           // if(pcData == "<NULL>")
             //   obj->pcPath = NULL;
            //else
              //  obj->pcPath = strdup(pcData.c_str());
            iss>>obj->size;
            iss>>obj->type;
            iss>>obj->uid;

            if(iss)
            {
                int x=0;
                while(iss)
                {
                    x=0;
                    iss>>x;
                    if(x ==0)
                        break;
                    obj->LInodes.push_back(x);
                }
            }
            M_struct[b]=obj;
        }
        return 1;
    }
#if 1
    for(map<int,struct st_node *>::iterator mt = M_struct.begin();\
            mt!= M_struct.end();mt++)
    {
        cout<<mt->first<<" "<<mt->second->name;
        if(mt->second->type=='D')
        {
            for(list<int>::iterator lit = mt->second->LInodes.begin();\
                    lit!=mt->second->LInodes.end(); lit++)
            {
                cout<<"\n"<<*lit;
            }
            
            
        }
        cout<<"\n";
        return 1;
    }
#endif
    return 0;
}
static int custom_rename (const char *oldpath, const char *newpath)
{
    string S_Old(oldpath);
    string S_New(newpath);
    char *basename1 = strdup(newpath);
    char *base = basename(basename1);
    if(M_Inode.count(S_Old) == 0)
    {
        return -ENOENT;
    }
    
    int inod = M_Inode[S_Old];
    struct st_node *pst = M_struct[inod];
    
    if(pst->type == 'F')
    {
        if(M_Inode.count(S_New)==1)
        {
            //File exists
            custom_unlink(newpath);
        }
        
        M_Inode[S_New] = M_Inode[S_Old];
        //M_struct[inod] = M_struct[inod];
        if(pst->name)
            free(pst->name);
        pst->name = strdup(base);
        M_Inode.erase(S_Old);
       // M_struct.erase(inod);
    }
    return 0;
}
void  custom_destroy(void *v)
{
    fstream fs;
    fs.open(DUMMY,ios::binary|ios::out|ios::ate);
    if(fs.is_open())
    {
        for(map<string,int>::iterator it= M_Inode.begin(); it!=M_Inode.end();it++)
        {
            fs<<it->first<<" "<<it->second<<"\n";
        }
        fs<<"INODE_MAP_OVER 100\n";
        
        for(map<int,struct st_node *>::iterator mt = M_struct.begin();\
            mt!= M_struct.end();mt++)
        {
            cout<<mt->first<<" "<<mt->second->name;
            fs<<mt->first<<" ";
            fs<<mt->second->atime<<" ";
            fs<<mt->second->ctime<<" ";
            fs<<mt->second->gid<<" ";
            fs<<mt->second->mode<<" ";
            fs<<mt->second->mtime<<" ";
            if(mt->second->pcDataBlock == NULL)
                fs<<"<NULL> ";
            else
            {
                fs<<"<FILE_DATA_BLOCK>\n";
                char *buf = (char *)malloc(mt->second->size -1);
                memcpy(buf,mt->second->pcDataBlock,mt->second->size-1);
                fs.write(buf,mt->second->size-1);
                free(buf);
                fs<<"\n<FILE_DATA_BLOCK>\n";
            }
            if(mt->second->name == NULL)
                fs<<"<NULL> ";
            else
                fs<<mt->second->name<<" ";
            fs<<mt->second->size<<" ";
            fs<<mt->second->type<<" ";
            fs<<mt->second->uid<<" ";
            if(mt->second->type=='D')
            {
                for(list<int>::iterator lit = mt->second->LInodes.begin();\
                        lit!=mt->second->LInodes.end(); lit++)
                {
                  //  cout<<"\n"<<*lit;
                    fs<<*lit<<" ";
                }
                  

            }
            fs<<"\n";
        }
        fs<<"STRUCT_MAP_OVER ";
        fs<<UsedMem<<" "<<gInodeCounter<<" ";
    }
    
}

static struct fuse_operations custom;

int main(int argc, char *argv[])
{
    
    char *params[3];
    params[0] = argv[0];
    params[1] = argv[1];
    params[2] = "-d";
    //params[2] = argv[4];
    
cout<<argv[3];
    if(argc ==4)
    {
        strcpy(DUMMY,argv[3]);
        cout<<"DUmmy = "<<DUMMY;
	 //cout<<"Less number of arguments\n";
        //return 0;
    }
    if(argc <3 || argc >4)
    {
        cout<<"Incorrect Number of arguments\n";
        exit(0);
    }
    //argc =2;
    TotalMemory = atoi(argv[2]);
    TotalMemory = TotalMemory * 1024 * 1024;
    int var =0;
    if(argc == 4)
        var = readRam(DUMMY);
    argc = 2;
    if(var == 0)
 	cout<<"Could not read\n"<<DUMMY;
    int tempmem = sizeof(struct st_node);
    
    if(tempmem > TotalMemory)
    {
        cout<<"Too less memory allocated for initial data structure to be loaded\n";
        return 0;
    }
    if(var == 0)
    {
        UsedMem += tempmem;
    
        M_Inode["/"]=gInodeCounter;

        struct st_node *pst = new struct st_node;
        pst->inode = gInodeCounter;
        pst->type = 'D';
        pst->size = 0;
        pst->gid = fuse_get_context()->gid;
        pst->uid = fuse_get_context()->uid;
        pst->name = NULL;
        pst->mode = 0777;
        pst->pcDataBlock = NULL;

        M_struct[gInodeCounter] = pst;
        gInodeCounter++;
    }
    
    custom.getattr = custom_getattr;
    custom.mkdir = custom_mkdir;
    custom.read = custom_read;
    custom.write = custom_write;
    custom.unlink = custom_unlink;
    custom.rmdir = custom_rmdir;
    custom.readdir = custom_readdir;
    custom.create = custom_create;
    custom.truncate = custom_truncate;
    custom.open = custom_open;
    custom.release = custom_release;
    custom.destroy = custom_destroy;
    custom.rename = custom_rename;
    int res = fuse_main(3, params, &custom,NULL);
}
