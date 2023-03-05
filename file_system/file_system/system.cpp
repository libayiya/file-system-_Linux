#include <stdlib.h>
#include <string.h>
#include "iostream"
#include "fstream"
#include "time.h"
using namespace std;

/**
 * FAT File System Structure
 * FAT �ļ�ϵͳ�ṹ
 *
 * |Start| End |    Name    |   Note            |
 * |-----|-----|------------|-------------------|
 * |  0  |  0  | Boot Block | ������           |
 * |-----|-----|------------|-------------------|
 * |  1  |  2  |    FAT1    | �ļ������         |
 * |-----|-----|------------|-------------------|
 * |  3  |  4  |    FAT2    | �ļ������-����  |
 * |-----|-----|------------|-------------------|
 * |  5  | 1000| DATA Area  | ������           |
 *
 */

 ///***** ����ĳ��� *****/
#define BLOCKSIZE       1024        // ���̿��С��ÿ����1024�ֽ�=1KB
#define SIZE            1024000     // ������̿ռ��С�������ֳ�1000�����̿飨������ռ1���̿飬FAT1,FAT2��ռ��2���̿飬�����̿��Ϊ��������
#define END             65535       // FAT�е��ļ�������־
#define FREE            0           // FAT���̿���б�־
#define ROOTBLOCKNUM    2           // ��Ŀ¼��ʼ��ռ�̿�����
#define MAXOPENFILE     10          // ���ͬʱ���ļ�����

#define MAX_TEXT_SIZE  10000        //������������ļ���С

/***** �������ݽṹ *****/

// �ļ����ƿ�
// ���ڼ�¼�ļ��������Ϳ�����Ϣ��ÿ���ļ�����һ��FCB����Ҳ���ļ���Ŀ¼�������
typedef struct FCB
{
    char filename[8];           // �ļ���
    char exname[3];             // �ļ���չ��
    unsigned char metadata;     // �ļ������ֶ� 1��ʾ�����ļ���0��ʾĿ¼�ļ�
    unsigned short time;        // �ļ�����ʱ��
    unsigned short date;        // �ļ���������
    unsigned short first;       // �ļ���ʼ�̿��
    unsigned long length;       // �ļ����ȣ��ֽ�����
    char free;                  // ��ʾĿ¼���Ƿ�Ϊ�գ���ֵΪ0����ʾ�գ�ֵΪ1����ʾ�ѷ���
} fcb;
// �ļ������
typedef struct FAT {
    unsigned short id;   //free����δ���䣬�������ĳ�ļ���һ���̿�Ŀ��
} fat;
// �û����ļ���
typedef struct USEROPEN {
    char filename[8];           // �ļ���
    char exname[3];             // �ļ���չ��
    unsigned char  metadata;    // �ļ������ֶ� 1��ʾ�����ļ���0��ʾĿ¼�ļ�
    unsigned short time;        // �ļ�����ʱ��
    unsigned short date;        // �ļ���������
    unsigned short first;       // �ļ���ʼ�̿��
    unsigned long  length;      // �ļ����ȣ��������ļ����ֽ�������Ŀ¼�ļ�������Ŀ¼�������
    char free;                  // ��ʾĿ¼���Ƿ�Ϊ�գ���ֵΪ0����ʾ�գ�ֵΪ1����ʾ�ѷ���
    //������������ļ�FCB�е����ݣ��������ļ�ʹ���еĶ�̬��Ϣ

    int  dirno;                 // ��Ӧ���ļ���Ŀ¼���ڸ�Ŀ¼�ļ��е��̿��
    int  diroff;                // ��Ӧ���ļ���Ŀ¼���ڸ�Ŀ¼�ļ���dirno�̿��е�Ŀ¼�����
    char dir[80];               // ��Ӧ���ļ����ڵ�Ŀ¼��������������ټ���ָ���ļ��Ƿ��Ѿ���
    int  filePtr;               // ��дָ�����ļ��е�λ��
    char fcbstate;              // �Ƿ��޸����ļ���FCB�����ݣ�����޸�����Ϊ1������Ϊ0
    char topenfile;             // ��ʾ���û��򿪱����Ƿ�Ϊ�գ���ֵΪ0����ʾΪ�գ������ʾ�ѱ�ĳ���ļ�ռ��
} useropen;
// ������ BootBlock�����������̵����������Ϣ
typedef struct BootBlock {
    char magic_number[8];       // �ļ�ϵͳ����
    char information[200];      // ������Ϣ��������̿��С�����̿�����
    unsigned short root;        // ��Ŀ¼�ļ�����ʼ�̿��
    unsigned char* startblock;  // �����������������ʼλ��
} block0;

/***** ȫ�ֱ������� *****/
unsigned char* v_addr0;             // ָ��������̵���ʼ��ַ
useropen openfilelist[MAXOPENFILE]; // �û����ļ������飬���ͬʱ��10���ļ�
int currfd;                         // ��¼��ǰĿ¼���ļ�������fd
unsigned char* startp;              // ��¼�����������������ʼλ��
char* FILENAME = "FileSys.txt";     // �ļ�ϵͳ��
unsigned char buffer[SIZE];         // ������


/***** �������� *****/
int  main();
void startsys();                                     // �����ļ�ϵͳ
void my_format();                                    // ���̸�ʽ������
void my_cd(char* dirname);                           // ���ڸ��ĵ�ǰĿ¼
void my_mkdir(char* dirname);                        // ������Ŀ¼
void my_rmdir(char* dirname);                        // ɾ����Ŀ¼
void my_ls();                                        // ��ʾĿ¼�е�����
int  my_create(char* filename);                      // �����ļ�
void my_rm(char* filename);                          // ɾ���ļ�
int  my_open(char* filename);                        // ���ļ�
int  my_close(int fd);                               // �ر��ļ�
int  my_write(int fd);                               // д�ļ�
int  do_write(int fd, char* text, int len, char wstyle);// ʵ��д�ļ�
int  my_read(int fd);                                // ���ļ�
int  do_read(int fd, int len, char* text);           // ʵ�ʶ��ļ�
void my_exitsys();                                   // �˳��ļ�ϵͳ

unsigned short getFreeBLOCK();                       // ��ȡһ�����еĴ��̿�
int  get_Free_Openfile();                            // ��ȡһ�����е��ļ��򿪱���
int  find_father_dir(int fd);                        // Ѱ��һ�����ļ��ĸ�Ŀ¼
void show_help();                                    // ��ȡ�������
void error(char* command);                           // ���������Ϣ

unsigned short int getFreeBLOCK() {
    fat* fat1 = (fat*)(v_addr0 + BLOCKSIZE);
    for (int i = 0; i < (int)(SIZE / BLOCKSIZE); i++) {
        if (fat1[i].id == FREE) {
            return i;
        }
    }
    return END;
}
int  get_Free_Openfile() {
    for (int i = 0; i < MAXOPENFILE; i++) {
        if (openfilelist[i].topenfile == 0) {
            openfilelist[i].topenfile = 1;
            return i;
        }
    }
    return -1;
}
int  find_father_dir(int fd) {
    for (int i = 0; i < MAXOPENFILE; i++) {
        if (openfilelist[i].first == openfilelist[fd].dirno) {
            return i;
        }
    }
    return -1;
}
void show_help() {
    printf("������\t\t\t�������\t\t\t�����\n\n");
    printf("cd\t\t\tĿ¼��(·����)\t\t\t�л���ǰĿ¼��ָ��Ŀ¼\n");
    printf("mkdir\t\t\tĿ¼��\t\t\t\t�ڵ�ǰĿ¼������Ŀ¼\n");
    printf("rmdir\t\t\tĿ¼��\t\t\t\t�ڵ�ǰĿ¼ɾ��ָ��Ŀ¼\n");
    printf("ls\t\t\t��\t\t\t\t��ʾ��ǰĿ¼�µ�Ŀ¼���ļ�\n");
    printf("create\t\t\t�ļ���\t\t\t\t�ڵ�ǰĿ¼�´���ָ���ļ�\n");
    printf("rm\t\t\t�ļ���\t\t\t\t�ڵ�ǰĿ¼��ɾ��ָ���ļ�\n");
    printf("open\t\t\t�ļ���\t\t\t\t�ڵ�ǰĿ¼�´�ָ���ļ�\n");
    printf("write\t\t\t��\t\t\t\t�ڴ��ļ�״̬�£�д���ļ�\n");
    printf("read\t\t\t�ļ���.��׺\t\t\t\t�ڴ��ļ�״̬�£���ȡ���ļ�\n");
    printf("close\t\t\t��\t\t\t\t�ڴ��ļ�״̬�£��رո��ļ�\n");
    printf("exit\t\t\t��\t\t\t\t�˳�ϵͳ\n\n");
}
void error(char* command) {
    printf("%s : ȱ�ٲ���\n", command);
    printf("���� 'help' ���鿴������ʾ.\n");
}
void set_fileopenlist(int fd, fcb* fcbPtr)  //��fcbptr�����ݸ�ֵ���ļ��򿪱�ĵ�fd����
{
    openfilelist[fd].metadata = fcbPtr->metadata;
    openfilelist[fd].filePtr = 0;
    openfilelist[fd].date = fcbPtr->date;
    openfilelist[fd].time = fcbPtr->time;
    strcpy(openfilelist[fd].filename, fcbPtr->filename);
    strcpy(openfilelist[fd].exname, fcbPtr->exname);
    openfilelist[fd].first = fcbPtr->first;
    openfilelist[fd].length = fcbPtr->length;
    openfilelist[fd].free = fcbPtr->free;
    //ǰ����FCB����
}


void startsys() {
    //	�� ����������̿ռ䣻
    //	�� ʹ��c���ԵĿ⺯��fopen()��myfsys�ļ������ļ����ڣ���ת�ۣ����ļ������ڣ��򴴽�֮��ת��
    //	�� ʹ��c���ԵĿ⺯��fread()����myfsys�ļ����ݵ��û��ռ��е�һ���������У����ж��俪ʼ��8���ֽ������Ƿ�Ϊ��10101010�����ļ�ϵͳħ����������ǣ���ת�ܣ�����ת�ݣ�
    //	�� �������������е����ݸ��Ƶ��ڴ��е�������̿ռ��У�ת��
    //	�� ����Ļ����ʾ��myfsys�ļ�ϵͳ�����ڣ����ڿ�ʼ�����ļ�ϵͳ����Ϣ��������my_format()�Ԣ������뵽��������̿ռ���и�ʽ��������ת�ޣ�

    //�ܽ�: 1. Ѱ��myfsys.txt�ļ�,������ڶ��ҿ�ͷ���ļ�ħ��,�Ͷ�ȡ��myvhard,���򴴽��ļ���д���ʼ����Ϣ
    //2. �����û����ļ���ĵ�һ������, ����Ϊ��Ŀ¼����, Ҳ����Ĭ�ϴ򿪸�Ŀ¼
    //3. ��ʼ��һ��ȫ�ֱ���
    v_addr0 = (unsigned char*)malloc(SIZE);  //����������̿ռ�
    //����ļ������ڻ��߿�ͷ�����ļ�ħ��,�����´����ļ�
    printf("��ʼ��ȡ�ļ�...");
    FILE* file;
    if ((file = fopen(FILENAME, "r")) != NULL) {
        fread(buffer, SIZE, 1, file);   //���������ļ���ȡ��������
        fclose(file);
        if (memcmp(buffer, "10101010", 8) == 0) {
            memcpy(v_addr0, buffer, SIZE);
            cout << "myfsys�ļ���ȡ�ɹ�!" << endl;
        }
        //���ļ����ǿ�ͷ�����ļ�ħ��
        else {
            cout << "myfsys�ļ�ϵͳ�����ڣ����ڿ�ʼ�����ļ�ϵͳ" << endl;
            my_format();
            memcpy(buffer, v_addr0, SIZE);
        }
    }
    else {
        cout << "myfsys�ļ�ϵͳ�����ڣ����ڿ�ʼ�����ļ�ϵͳ" << endl;
        my_format();
        memcpy(buffer, v_addr0, SIZE);
    }


    //	�� ��ʼ���û����ļ���������0�������Ŀ¼�ļ�ʹ�ã�����д��Ŀ¼�ļ��������Ϣ�����ڸ�Ŀ¼û���ϼ�Ŀ¼��
    // ���Ա����е�dirno��diroff�ֱ���Ϊ5����Ŀ¼������ʼ��ţ���0������ptrcurdirָ��ָ����û����ļ����
    //	�� ����ǰĿ¼����Ϊ��Ŀ¼��
    fcb* root;
    root = (fcb*)(v_addr0 + 5 * BLOCKSIZE);
 /*   strcpy(openfilelist[0].filename, root->filename);     //�Ľ�
    strcpy(openfilelist[0].exname, root->exname);
    openfilelist[0].metadata = root->metadata;
    openfilelist[0].time = root->time;
    openfilelist[0].date = root->date;
    openfilelist[0].first = root->first;
    openfilelist[0].length = root->length;
    openfilelist[0].free = root->free;*/


    openfilelist[0].dirno = 5;
    openfilelist[0].diroff = 0;
    strcpy(openfilelist[0].dir, "\\root\\");
    openfilelist[0].filePtr = 0;
    openfilelist[0].fcbstate = 0;
    openfilelist[0].topenfile = 1;

    //��ʼ��ȫ�ֱ���
    //startpָ���������Ŀ�ͷ
    startp = ((block0*)v_addr0)->startblock;
    currfd = 0;
    return;
}
void my_format() {
    //  �� ��������̵�һ������Ϊ�����飬��ʼ��8���ֽ����ļ�ϵͳ��ħ������Ϊ��10101010������֮��д���ļ�ϵͳ��������Ϣ����FAT���С��λ�á���Ŀ¼��С��λ�á��̿��С���̿���������������ʼλ�õ���Ϣ��
    //	�� �����������������ȫһ����FAT�����ڼ�¼�ļ���ռ�ݵĴ��̿鼰����������̿�ķ��䣬ÿ��FATռ���������̿飻����ÿ��FAT�У�ǰ��5��������Ϊ�ѷ��䣬����995��������Ϊ���У�
    //	�� �ڵڶ���FAT�󴴽���Ŀ¼�ļ�root�����������ĵ�1�飨��������̵ĵ�6�飩�������Ŀ¼�ļ����ڸô����ϴ������������Ŀ¼���.���͡�..���������ݳ����ļ�����֮ͬ�⣬�����ֶ���ȫ��ͬ��
    //	�� ����������е����ݱ��浽myfsys�ļ��У�ת��
    //	�� ʹ��c���ԵĿ⺯��fclose()�ر�myfsys�ļ���

    //�ܽ�: 1. ����������(1���̿�)
    //2. ����FAT1(2���̿�) FAT2(2���̿�)
    //3. ���ø�Ŀ¼�ļ�����������Ŀ¼��.��..(��Ŀ¼�ļ�ռһ���̿�,����Ŀ¼������д������̿������)��1���̿顢��6�飩

    //��������Ϣ
    block0* boot = (block0*)v_addr0;
    strcpy(boot->magic_number, "10101010");
    strcpy(boot->information, "�ļ�ϵͳ,�����䷽ʽ:FAT,���̿ռ����:�����FAT��λʾͼ,Ŀ¼�ṹ:���û��༶Ŀ¼�ṹ.");
    boot->root = 5;
    boot->startblock = v_addr0 + BLOCKSIZE * 5;

    //��������FAT����Ϣ
    fat* fat1 = (fat*)(v_addr0 + BLOCKSIZE);
    for (int i = 0; i < 5; i++) {  //ǰ5������Ϊ�ѷ��� ��995������Ϊfree
        fat1[i].id = END;
    }
    for (int i = 5; i < 1000; i++) {
        fat1[i].id = FREE;
    }
    fat* fat2 = (fat*)(v_addr0 + BLOCKSIZE * 3);
    memcpy(fat2, fat1, BLOCKSIZE);

    //5���̿鱻��Ŀ¼��ռ����
    fat1[5].id = fat2[5].id = END;

    //��Ŀ¼����fcb,����Ŀ¼��., ָ���Լ�
    fcb* root = (fcb*)(v_addr0 + BLOCKSIZE * 5);
    strcpy(root->filename, ".");
    strcpy(root->exname, "di");
    root->metadata = 0;

    time_t rawTime = time(NULL);
    struct tm* time = localtime(&rawTime);
    //root->time ��unsigned short int���͵�����, 32λ�������³�16λ,64λ�������³�32λ
    //���￼��32λ������, ���������ı�ʾһ��ʱ��������ǲ�������,����,����������һ��
    //����Сʱռ5λ,����ռ6λ,��ռ5λ
    root->time = time->tm_hour * 2048 + time->tm_min * 32 + time->tm_sec / 2;

    root->date = (time->tm_year - 100) * 512 + (time->tm_mon + 1) * 32 + (time->tm_mday);
    root->first = 5;
    root->free = 1;
    root->length = 2 * sizeof(fcb);

    //root2 ָ���Ŀ¼���ĵڶ���fcb,������Ŀ¼��..,��Ϊ��Ŀ¼��û���ϼ�Ŀ¼,����ָ���Լ�
    fcb* root2 = root + 1;
    memcpy(root2, root, sizeof(fcb));
    strcpy(root2->filename, "..");

    //�Ӹ�Ŀ¼�ĵڶ���fcb��ʼ��������Ŀ¼�Ϊ��
    for (int i = 2; i < int(BLOCKSIZE / sizeof(fcb)); i++) {
        root2++;
        strcpy(root2->filename, "");
        root2->free = 0;
    }

    //д���ļ�����ȥ
    FILE* fp = fopen(FILENAME, "w");
    fwrite(v_addr0, SIZE, 1, fp);
    fclose(fp);
}
void my_cd(char* dirname) {
    //�ܽ�: 1. �����ǰ��Ŀ¼�ļ���,��ô��Ҫ�����Ŀ¼�ļ���ȡ��buf��, Ȼ���������ļ����fcb��û��ƥ��dirname��Ŀ¼��(���ұ�����Ŀ¼�ļ�)
    //          �����,�Ǿ���openfilelist��ȡһ�����ļ�����,�����dirname���Ŀ¼�ļ���fcbд��ȥ,Ȼ���л�currfd=fd
    //          ���������Ǵ�һ��Ŀ¼
    if (openfilelist[currfd].metadata == 1) {
        cout << "�����ļ��ﲻ��ʹ��cd, Ҫ���˳��ļ�, ����closeָ��" << endl;
        return;
    }
    //�����Ŀ¼�ļ�
    else {
        //Ѱ��Ŀ¼�ļ�������û��ƥ�������, �Ȱ�Ŀ¼�ļ�����Ϣ��ȡ��buf��
        char* buf = (char*)malloc(MAX_TEXT_SIZE);
        openfilelist[currfd].filePtr = 0;
        do_read(currfd, openfilelist[currfd].length, buf);
        int i = 0;
        fcb* fcbPtr = (fcb*)buf;
        for (; i < int(openfilelist[currfd].length / sizeof(fcb)); i++, fcbPtr++) {
            if (strcmp(fcbPtr->filename, dirname) == 0 && fcbPtr->metadata == 0) {
                break;
            }
        }
        if (i == int(openfilelist[currfd].length / sizeof(fcb)))    //�Ľ�
        {
            cout << "���������Ŀ¼�ļ���" << endl;
            return;
        }
        //������cd���ļ�
        if (strcmp(fcbPtr->exname, "di") != 0) {
            cout << "������cd��Ŀ¼�ļ�!" << endl;
            return;
        }
        //���cd ��һ��Ŀ¼�ļ�, ��ô�ж���.����..�������ļ�,��������ļ�������Ŀ¼�ļ���openfilelist��
        else {
            //cd .�����з�Ӧ
            if (strcmp(fcbPtr->filename, ".") == 0) {
                return;
            }
            //cd ..��Ҫ�ж������ǲ��Ǹ�Ŀ¼, ����Ǹ�Ŀ¼,������, ����,������һ��
            else if (strcmp(fcbPtr->filename, "..") == 0) {
                if (currfd == 0) {
                    return;
                }
                else {
                    currfd = my_close(currfd);  //my_close�������ص�ǰfd����һ��fd
                    return;
                }
            }
            //cd ��ǰĿ¼�µ����ļ�
            else {
                int fd = get_Free_Openfile();   //�ҵ��յ��û��򿪱���
                if (fd == -1) { //û�пյı�����
                    return;
                }
                else {
                    set_fileopenlist(fd, fcbPtr);
                    openfilelist[fd].fcbstate = 0;
                    openfilelist[fd].topenfile = 1;
                    openfilelist[fd].dirno = openfilelist[currfd].first;
                    // �޸� openfilelist[fd].dir[fd] = openfilelist[currfd].dir[currfd] + dirname;
                    strcpy(openfilelist[fd].dir,
                        (char*)(string(openfilelist[currfd].dir) + string(dirname) + string("\\")).c_str());
                    openfilelist[fd].diroff = i;
                    currfd = fd;
                }
            }
        }

    }

}
void my_mkdir(char* dirname) {
    // 1. �ж�dirname�Ƿ�Ϸ�
    // 2. �򿪵�ǰĿ¼��Ŀ¼�ļ�,�����Ƿ�����
    // 3. ռ��һ���̿�, ��FAT��Ҫд������̿鱻ռ����
    // 4. ռ��һ�����ļ�����,������Ϊһ��Ҫ�����Ŀ¼�ļ�����д��Ĭ�ϵ�.��..���������ļ�Ŀ¼��
    //   ������do_write,����Ҫ�ȴ�����ļ���.
    //   д��.��..�����ļ�Ŀ¼��½���Ŀ¼�ļ���
    // 5. ������openfilelist 
    // 6. �޸ĸ�Ŀ¼�ļ��Ĵ�С,��Ϊ�����һ���ļ�Ŀ¼��
    // �� ��\a\���Ŀ¼�´�����b���Ŀ¼, ��ôa���Ŀ¼�ļ��Ĵ�СҪ+=sizeof(fcb)

    //�ж�dirname�Ƿ�Ϸ�
    char* fname = strtok(dirname, ".");
    char* exname = strtok(NULL, ".");
    if (exname) {
        cout << "�����������׺��!" << endl;
        return;
    }
    char text[MAX_TEXT_SIZE];   //������������ļ���С
    openfilelist[currfd].filePtr = 0;   //�û����ļ������飬currfd��¼��ǰĿ¼���ļ�������
    int fileLen = do_read(currfd, openfilelist[currfd].length, text);//Ҫ���ȡ�ĳ���Ϊlength������ֵΪʵ�ʶ�ȡ�ĳ��ȡ�Ҫ�����ļ����ȴ��뵽text��
    //text������ݾ���һ����fcb
    fcb* fcbPtr = (fcb*)text;
    for (int i = 0; i < (int)(fileLen / sizeof(fcb)); i++) {
        if (strcmp(dirname, fcbPtr[i].filename) == 0 && fcbPtr->metadata == 0) {
            cout << "Ŀ¼���Ѿ�����!" << endl;
            return;
        }
    }
    //�ڴ��ļ�������һ�����ļ�����
    int fd = get_Free_Openfile();
    if (fd == -1) {
        cout << "���ļ�����ȫ����ռ��" << endl;
        return;
    }
    //��FAT����һ�����̿�
    unsigned short int blockNum = getFreeBLOCK();
    if (blockNum == END) {
        cout << "�̿��Ѿ�����" << endl;
        openfilelist[fd].topenfile = 0;
        return;
    }
    fat* fat1 = (fat*)(v_addr0 + BLOCKSIZE);
    fat* fat2 = (fat*)(v_addr0 + BLOCKSIZE * 3);
    fat1[blockNum].id = END;
    fat2[blockNum].id = END;
    //�ڵ�ǰĿ¼�������һ������Ҫ��Ŀ¼��
    int i = 0;
    for (; i < (int)(fileLen / sizeof(fcb)); i++) {
        if (fcbPtr[i].free == 0) {
            break;
        }
    }
    openfilelist[currfd].filePtr = i * sizeof(fcb);
    //�޸���fcb,fcbstate��1
    openfilelist[currfd].fcbstate = 1;
    //�޸��½���Ŀ¼��,��fcb����
    //��Ϊ��������ģ���ļ�ϵͳ,����Ҫ��д����ʱ��fcb��,Ȼ����do_writeתд��������
    fcb* fcbtmp = new fcb;
    fcbtmp->metadata = 0;
    time_t rawtime = time(NULL);
    struct tm* time = localtime(&rawtime);
    fcbtmp->date = (time->tm_year - 100) * 512 + (time->tm_mon + 1) * 32 + (time->tm_mday);
    fcbtmp->time = (time->tm_hour) * 2048 + (time->tm_min) * 32 + (time->tm_sec) / 2;
    strcpy(fcbtmp->filename, dirname);
    strcpy(fcbtmp->exname, "di");
    fcbtmp->first = blockNum;
    fcbtmp->length = 2 * sizeof(fcb);   //�½���Ŀ¼�ļ���������������Ŀ¼��.��..
    fcbtmp->free = 1;
    //��do_write��fcbtmpд�뵽Ŀ¼�ļ���
    do_write(currfd, (char*)fcbtmp, fcbtmp->length, 1);

    //�ڶ�Ӧ���̿���������������Ŀ¼.��..
 //ͬ������Ҳ����ֱ��д��fcbPtr��,ֻ����fcbTmpд���ڴ���,Ȼ����do_writeתд���ļ�
    strcpy(fcbtmp->filename, ".");
    strcpy(fcbtmp->exname, "di");
    do_write(fd, (char*)fcbtmp, sizeof(fcb), 1);
    //����..Ŀ¼
    fcb* fcbtmp2 = new fcb;
    memcpy(fcbtmp2, fcbtmp, sizeof(fcb));
    strcpy(fcbtmp2->filename, "..");
    fcbtmp2->first = openfilelist[currfd].first;
    fcbtmp2->length = openfilelist[currfd].length;
    fcbtmp2->date = openfilelist[currfd].date;
    fcbtmp2->time = openfilelist[currfd].time;
    do_write(fd, (char*)fcbtmp2, sizeof(fcb), 1);   //����д����ʱ,fd��length�ͽ�����Ӧ�Ӹı���

    //���ô��ļ�����
    set_fileopenlist(fd, fcbtmp);
    strcpy(openfilelist[fd].filename, dirname);
    openfilelist[fd].fcbstate = 0;
    openfilelist[fd].topenfile = 1;
    openfilelist[fd].dirno = openfilelist[currfd].first;
    openfilelist[fd].diroff = i;
    openfilelist[fd].topenfile = 1;
    // �޸� openfilelist[fd].dir[fd] = openfilelist[currfd].dir[currfd] + dirname;
    strcpy(openfilelist[fd].dir, (char*)(string(openfilelist[currfd].dir) + string(dirname) + string("\\")).c_str());

    my_close(fd);
    //���±�currfdĿ¼�ļ���fcb
    fcbPtr->length = openfilelist[currfd].length;   //������дcurrfd��ʱ����Ѿ������ĳ��ȸı���
    openfilelist[currfd].filePtr = 0;
    do_write(currfd, (char*)fcbPtr, fcbPtr->length, 1);
    openfilelist[currfd].fcbstate = 1;
    delete fcbtmp;
    delete fcbtmp2;
}
void my_rmdir(char* dirname) {
    //����,���ǲ�����ɾ���ǿյ�Ŀ¼�ļ�
    //1. �ѵ�ǰĿ¼��Ŀ¼�ļ���ȡ��buf��
    //2. �����������û��ƥ��dirname��Ŀ¼�ļ�
    //3. ɾ�����Ŀ¼�ļ�ռ�õ������̿�(Ҳ���ǰ���ռ�õ�fatȫ���ͷ�,���ұ��ݵ�fat2)
    //4. ���µ�ǰĿ¼�ļ�,��dirname��Ӧ��fcb���,���Ҹ��µ�ǰĿ¼�ļ��Ĵ�С
    //�� ����\a\Ŀ¼��,ɾ��b���Ŀ¼,��ôɾ����֮��,a���Ŀ¼�ļ��Ĵ�С(length)Ҫ-=sizeof(fcb)
    //   ����,b���Ŀ¼�ļ�����һ��fcb��a���Ŀ¼�ļ����,�����fcbҲɾ��

    char* fname = strtok(dirname, ".");
    char* exname = strtok(NULL, ".");
    //������ɾ��.��..����������Ŀ¼�ļ�
    if (strcmp(dirname, ".") == 0 || strcmp(dirname, "..") == 0) {
        cout << "����ɾ��" << dirname << "�������Ŀ¼��" << endl;
        return;
    }
    if (exname) {
        cout << "ɾ��Ŀ¼�ļ����������׺��!" << endl;
        return;
    }
    //��ȡcurrfd��Ӧ��Ŀ¼�ļ���buf
    char buf[MAX_TEXT_SIZE];
    openfilelist[currfd].filePtr = 0;
    do_read(currfd, openfilelist[currfd].length, buf);// ����currfd�ҵ���ʼ�̿�ţ�Ҫ�����ļ����뵽buf��
    int i;
    fcb* fcbPtr = (fcb*)buf;
    //Ѱ�ҽ�������ֵ��ļ�Ŀ¼��
    for (i = 0; i < int(openfilelist[currfd].length / sizeof(fcb)); i++, fcbPtr++) {
        if (strcmp(fcbPtr->filename, fname) == 0 && fcbPtr->metadata == 0) {    //metddata=0��ʾ��Ŀ¼�ļ�
            break;
        }
    }
    if (i == int(openfilelist[currfd].length / sizeof(fcb))) {  //����ҵ������һ��֮��˵��û������ļ�
        cout << "û�����Ŀ¼�ļ�" << endl;
        return;
    }
    //�ж����Ŀ¼�ļ���,�����û��,������ɾ��û����յ�Ŀ¼
    if (fcbPtr->length > 2 * sizeof(fcb)) { //������������Ŀ¼�ļ�.��.. ���������ļ����Ͳ���ɾ��
        cout << "����������Ŀ¼�µ������ļ�,��ɾ��Ŀ¼�ļ�" << endl;
        return;
    }
    //������Ŀ¼��ռ�ݵ�FAT
    int blockNum = fcbPtr->first;   //�ļ���ʼ�̿��
    fat* fat1 = (fat*)(v_addr0 + BLOCKSIZE);    //������̵���ʼ��ַ+���̿��С
    int next = 0;
    while (1) {
        next = fat1[blockNum].id;
        fat1[blockNum].id = END;    //fat.id=free����δ���䣬��������ĳ�ļ���һ���̿�Ŀ��
        if (next != END) {
            blockNum = next;    //nextһֱ������һ���̿�Ŀ�ţ�һֱ�����ű��������end��fat���е��ļ�������־
        }
        else {
            break;
        }
    }
    //����fat2
    fat* fat2 = (fat*)(v_addr0 + BLOCKSIZE * 3);
    memcpy(fat2, fat1, sizeof(fat));

    //�޸����fcbΪ��
    fcbPtr->date = 0;
    fcbPtr->time = 0;
    fcbPtr->exname[0] = '\0';
    fcbPtr->filename[0] = '\0';
    fcbPtr->first = 0;
    fcbPtr->free = 0;
    fcbPtr->length = 0;
    //д��������ȥ, ����fcb����Ϊ��
    openfilelist[currfd].filePtr = i * sizeof(fcb); //fileptr�����дָ�����ļ��е�λ��
    do_write(currfd, (char*)fcbPtr, sizeof(fcb), 1); //��fcptrb����д�뵽buf���ٰ�buf����д�뵽���̿飻1:����д:Ҳ���Ǵ�filePtrָ������ݼ�������д
    openfilelist[currfd].length -= sizeof(fcb);
    //����.Ŀ¼��ĳ���
    fcbPtr->length = openfilelist[currfd].length;
    openfilelist[currfd].filePtr = 0;
    do_write(currfd, (char*)fcbPtr, sizeof(fcb), 1);
    openfilelist[currfd].fcbstate = 1;  //�ļ�fcb�޸ı�־��Ϊ1����ʾ���޸�
}
void my_ls() {
    //�� ����do_read()������ǰĿ¼�ļ����ݵ��ڴ棻
    //	�� ��������Ŀ¼�ļ�����Ϣ����һ���ĸ�ʽ��ʾ����Ļ�ϣ�
    //	�� ���ء�

    if (openfilelist[currfd].metadata == 1) {
        cout << "�������ļ��ﲻ��ʹ��ls" << endl;
        return;
    }
    char buf[MAX_TEXT_SIZE];

    openfilelist[currfd].filePtr = 0;   //��дָ��λ����Ϊ��ʼ��
    do_read(currfd, openfilelist[currfd].length, buf);  //����ǰĿ¼�µ��ļ�����Buf��

    fcb* fcbPtr = (fcb*)buf;
    printf("name\tsize \ttype\t\tdate\t\ttime\n");
    for (int i = 0; i < (int)(openfilelist[currfd].length / sizeof(fcb)); i++) {
        if (fcbPtr->free == 1) {
            //Ŀ¼�ļ�
            //ͬ��,���ռ7λ,�·�ռ4λ,����ռ5λ
            //Сʱռ5λ,����ռ6λ,��ռ5λ	
            if (fcbPtr->metadata == 0) {
                printf("%s\t%dB\t<DIR>\t%d/%d/%d\t%02d:%02d:%02d\n",
                    fcbPtr->filename, fcbPtr->length,
                    (fcbPtr->date >> 9) + 2000,     //�ļ��������ڣ��꣬16Bit
                    (fcbPtr->date >> 5) & 0x000f,
                    (fcbPtr->date) & 0x001f,
                    (fcbPtr->time >> 11),
                    (fcbPtr->time >> 5) & 0x003f,
                    (fcbPtr->time) & 0x001f * 2);
            }
            else {
                // ��ͨ�ļ�length - 2 ����Ϊĩβ��/n��/0�����ַ�
                unsigned int length = fcbPtr->length;
                if (length != 0)
                    length -= 2;
                printf("%s.%s\t%dB\t<File>\t%d/%d/%d\t%02d:%02d:%02d\n",
                    fcbPtr->filename,
                    fcbPtr->exname,
                    length,
                    (fcbPtr->date >> 9) + 2000,
                    (fcbPtr->date >> 5) & 0x000f,
                    (fcbPtr->date) & 0x001f,
                    (fcbPtr->time >> 11),
                    (fcbPtr->time >> 5) & 0x003f,
                    (fcbPtr->time) & 0x001f * 2);
            }
        }
        fcbPtr++;
    }
}
int  my_create(char* filename) {
    //  �� Ϊ���ļ�����һ�����д��ļ�������û�п��б����򷵻�-1������ʾ������Ϣ��
    //	�� �����ļ��ĸ�Ŀ¼�ļ���û�д򿪣������my_open()�򿪣�����ʧ�ܣ����ͷŢ���Ϊ�½��ļ�����Ŀ����ļ��򿪱������-1������ʾ������Ϣ��
    //	�� ����do_read()�����ø�Ŀ¼�ļ����ݵ��ڴ棬����Ŀ¼�����ļ��Ƿ����������������ͷŢ��з���Ĵ��ļ����������my_close()�رբ��д򿪵�Ŀ¼�ļ���Ȼ�󷵻�-1������ʾ������Ϣ��
    //	�� ���FAT�Ƿ��п��е��̿飬������Ϊ���ļ�����һ���̿飬�����ͷŢ��з���Ĵ��ļ����������my_close()�رբ��д򿪵�Ŀ¼�ļ�������-1������ʾ������Ϣ��
    //	�� �ڸ�Ŀ¼��Ϊ���ļ�Ѱ��һ�����е�Ŀ¼���Ϊ��׷��һ���µ�Ŀ¼��;���޸ĸ�Ŀ¼�ļ��ĳ�����Ϣ��������Ŀ¼�ļ����û����ļ������е�fcbstate��Ϊ1��
    //	�� ׼�������ļ���FCB�����ݣ��ļ�������Ϊ�����ļ�������Ϊ0���Ը���д��ʽ����do_write()������д�����з��䵽�Ŀ�Ŀ¼���У�
    //	�� Ϊ���ļ���д���з��䵽�Ŀ��д��ļ����fcbstate�ֶ�ֵΪ0����дָ��ֵΪ0��
    //	�� ����my_close()�رբ��д򿪵ĸ�Ŀ¼�ļ���
    //	�� �����ļ��Ĵ��ļ����������Ϊ���ļ����������ء�

    char* fname = strtok(filename, ".");
    char* exname = strtok(NULL, ".");
    if (strcmp(fname, "") == 0) {
        cout << "�������ļ���!" << endl;
        return -1;
    }
    if (!exname) {
        cout << "�������׺��!" << endl;
        return -1;
    }
    if (openfilelist[currfd].metadata == 1) {
        cout << "�����ļ��²�����ʹ��create" << endl;
        return -1;
    }
    //��ȡcurrfd��Ӧ���ļ�
    openfilelist[currfd].filePtr = 0;
    char buf[MAX_TEXT_SIZE];
    do_read(currfd, openfilelist[currfd].length, buf);
    int i;
    fcb* fcbPtr = (fcb*)(buf);
    //������û�������ļ�
    for (i = 0; i < int(openfilelist[currfd].length / sizeof(fcb)); i++, fcbPtr++) {
        if (strcmp(fcbPtr->filename, filename) == 0 && strcmp(fcbPtr->exname, exname) == 0) {
            cout << "����ͬ���ļ�����!" << endl;
            return -1;
        }
    }
    //Ѱ�ҿյ�fcb��
    fcbPtr = (fcb*)(buf);
    fcb* debug = (fcb*)(buf);
    for (i = 0; i < int(openfilelist[currfd].length / sizeof(fcb)); i++, fcbPtr++) {
        if (fcbPtr->free == 0)
            break;
    }
    if (i == int(openfilelist[currfd].length / sizeof(fcb))) {  //����ҵ������һ��֮��˵��û������ļ�
        cout << "û�п���Ŀ¼�" << endl;
        return;
    }
    //ȡһ�����д��̿�
    int blockNum = getFreeBLOCK();
    if (blockNum == -1) {
        return -1;
    }
    //����
    fat* fat1 = (fat*)(v_addr0 + BLOCKSIZE);
    fat1[blockNum].id = END;
    fat* fat2 = (fat*)(v_addr0 + BLOCKSIZE * 3);
    memcpy(fat2, fat1, BLOCKSIZE * 2);
    //��fcb��д��Ϣ
    strcpy(fcbPtr->filename, filename);
    strcpy(fcbPtr->exname, exname);
    time_t rawtime = time(NULL);
    struct tm* time = localtime(&rawtime);
    fcbPtr->date = (time->tm_year - 100) * 512 + (time->tm_mon + 1) * 32 + (time->tm_mday);
    fcbPtr->time = (time->tm_hour) * 2048 + (time->tm_min) * 32 + (time->tm_sec) / 2;
    fcbPtr->first = blockNum;
    fcbPtr->free = 1;
    fcbPtr->length = 0;
    fcbPtr->metadata = 1;
    openfilelist[currfd].filePtr = i * sizeof(fcb);
    fcbPtr->length = openfilelist[currfd].length;
 //   openfilelist[currfd].filePtr = 0;
    do_write(currfd, (char*)fcbPtr, sizeof(fcb), 1);
    openfilelist[currfd].fcbstate = 1;
}
void my_rm(char* filename) {
    //�� ������\a\Ŀ¼��
    //1. ��ȡ��ǰĿ¼��Ŀ¼�ļ�(a)��buf��(buf����һ����fcb)
    //2. ��buf��Ѱ��ƥ��filename�������ļ�
    //3. �������ļ�ռ�ݵ��̿�(Ҳ�����ͷ���ռ�ݵ�fat,���ұ���)
    //4. �ڸ�Ŀ¼�ļ���,ɾ��filename��Ӧ��fcb   (a��ɾ��b��fcb)
    //5. ���¸�Ŀ¼�ļ��ĳ���(a��Ŀ¼�ļ�����)

    char* fname = strtok(filename, ".");
    char* exname = strtok(NULL, ".");
    if (!exname) {
        cout << "�������׺��!" << endl;
        return;
    }
    if (strcmp(exname, "di") == 0) {
        cout << "����ɾ��Ŀ¼��" << endl;
        return;
    }
    //��ȡcurrfd��Ӧ��Ŀ¼�ļ���buf
    char buf[MAX_TEXT_SIZE];
    openfilelist[currfd].filePtr = 0;
    do_read(currfd, openfilelist[currfd].length, buf);
    int i;
    fcb* fcbPtr = (fcb*)buf;
    //Ѱ�ҽ�������ֵ��ļ�Ŀ¼��
    for (i = 0; i < int(openfilelist[currfd].length / sizeof(fcb)); i++, fcbPtr++) {
        if (strcmp(fcbPtr->filename, fname) == 0 && strcmp(fcbPtr->exname, exname) == 0) {
            break;
        }
    }
    if (i == int(openfilelist[currfd].length / sizeof(fcb))) {
        cout << "û������ļ�" << endl;
        return;
    }
    //������Ŀ¼��ռ�ݵ�FAT
    int blockNum = fcbPtr->first;
    fat* fat1 = (fat*)(v_addr0 + BLOCKSIZE);
    int next = 0;
    while (1) {
        next = fat1[blockNum].id;
        fat1[blockNum].id = FREE;
        if (next != END) {
            blockNum = next;
        }
        else {
            break;
        }
    }
    //����fat2
    fat1 = (fat*)(v_addr0 + BLOCKSIZE);
    fat* fat2 = (fat*)(v_addr0 + BLOCKSIZE * 3);
    memcpy(fat2, fat1, sizeof(fat));
    //�޸����fcbΪ��
    fcbPtr->date = 0;
    fcbPtr->time = 0;
    fcbPtr->exname[0] = '\0';
    fcbPtr->filename[0] = '\0';
    fcbPtr->first = 0;
    fcbPtr->free = 0;
    fcbPtr->length = 0;
    //д��������ȥ, ����fcb����Ϊ��
    openfilelist[currfd].filePtr = i * sizeof(fcb);
    do_write(currfd, (char*)fcbPtr, sizeof(fcb), 1);
    openfilelist[currfd].length -= sizeof(fcb);
    //����.Ŀ¼��ĳ���
    fcbPtr = (fcb*)buf;
    fcbPtr->length = openfilelist[currfd].length;
    openfilelist[currfd].filePtr = 0;
    do_write(currfd, (char*)fcbPtr, sizeof(fcb), 1);
    openfilelist[currfd].fcbstate = 1;
}
int  my_open(char* filename) {
    //�ܽ�: �ѵ�ǰĿ¼�ļ���ȡ��buf��,buf�������һ����fcb��,����Щfcb��Ѱ��ƥ����filename�������ļ�
    //      Ȼ���openfilelist��ȡ��һ�����ļ�����,����������ļ�����Ϣд��ȥ,�л�currfd=fd�����Ǵ��ļ���

    //�ѵ�ǰĿ¼�ļ���ȡ��buf��
    char buf[MAX_TEXT_SIZE];
    openfilelist[currfd].filePtr = 0;
    do_read(currfd, openfilelist[currfd].length, buf);
    char* fname = strtok(filename, ".");
    char* exname = strtok(NULL, ".");
    if (!exname) {
        cout << "�������׺��" << endl;
        return -1;
    }
    int i;
    fcb* fcbPtr = (fcb*)buf;
    //Ѱ�Ҵ��ļ�
    for (i = 0; i < int(openfilelist[currfd].length / sizeof(fcb)); i++, fcbPtr++) {
        if (strcmp(fcbPtr->filename, fname) == 0 && strcmp(fcbPtr->exname, exname) == 0 && fcbPtr->metadata == 1) {
            break;
        }
    }
    if (i == int(openfilelist[currfd].length / sizeof(fcb))) {
        cout << "�����ڴ��ļ�!" << endl;
        return -1;
    }
    //Ϊ������һ�����ļ�����
    int fd = get_Free_Openfile();
    if (fd == -1) {
        cout << "�û����ļ����Ѿ�����" << endl;
        return -1;
    }
    openfilelist[fd].metadata = 1;
    openfilelist[fd].filePtr = 0;
    openfilelist[fd].date = fcbPtr->date;
    openfilelist[fd].time = fcbPtr->time;
    strcpy(openfilelist[fd].exname, exname);
    strcpy(openfilelist[fd].filename, fname);
    openfilelist[fd].length = fcbPtr->length;
    openfilelist[fd].first = fcbPtr->first;
    strcpy(openfilelist[fd].dir, (string(openfilelist[currfd].dir) + string(filename)).c_str());
    openfilelist[fd].dirno = openfilelist[currfd].first;
    openfilelist[fd].diroff = i;
    openfilelist[fd].free = 1;
    openfilelist[fd].topenfile = 1;
    openfilelist[fd].fcbstate = 0;


    currfd = fd;
    return 1;
}
int  my_close(int fd) {
    //�� ���fd����Ч�ԣ�fd���ܳ����û����ļ����������������±꣩�������Ч�򷵻�-1��
    //	�� ����û����ļ�������е�fcbstate�ֶε�ֵ�����Ϊ1����Ҫ�����ļ���FCB�����ݱ��浽��������ϸ��ļ���Ŀ¼���У�
    //    �����ǣ��򿪸��ļ��ĸ�Ŀ¼�ļ����Ը���д��ʽ����do_write()�����ر��ļ���FCBд�븸Ŀ¼�ļ�����Ӧ�̿��У�
    //	�� ���ո��ļ�ռ�ݵ��û����ļ�����������ղ�����������topenfile�ֶ���Ϊ0��
    //	�� ���ء�

    if (fd > MAXOPENFILE || fd < 0) {
        cout << "������������ļ�" << endl;
        return-1;
    }
    else {
        //�жϸ�Ŀ¼�ļ��Ƿ����, �����ڱ���
        int fatherFd = find_father_dir(fd);
        if (fatherFd == -1) {
            cout << "��Ŀ¼������!" << endl;
            return -1;
        }
        //fcb���޸���, Ҫд��ȥ
        //�Ǿ�Ҫ�ȰѸ�Ŀ¼�ļ��Ӵ����ж�ȡ��buf��,Ȼ��Ѹ��º��fcb����д��buf��, Ȼ���ٴ�bufд��������
        //д�ص����̵�ʱ��,ֻҪ���α�filePtrָ������ļ��Ķ�Ӧfcb��λ��,Ȼ��д��fcb�ͺ���
        //����ֻҪдһ��fcb��С�����ݾ�����
        if (openfilelist[fd].fcbstate == 1) {
            char buf[MAX_TEXT_SIZE];
            do_read(fatherFd, openfilelist[fatherFd].length, buf);
            //����fcb����
            fcb* fcbPtr = (fcb*)(buf + sizeof(fcb) * openfilelist[fd].diroff);
            strcpy(fcbPtr->exname, openfilelist[fd].exname);
            strcpy(fcbPtr->filename, openfilelist[fd].filename);
            fcbPtr->first = openfilelist[fd].first;
            fcbPtr->free = openfilelist[fd].free;
            fcbPtr->length = openfilelist[fd].length;
            openfilelist[fatherFd].filePtr = 0;
            fcbPtr->time = openfilelist[fd].time;
            fcbPtr->date = openfilelist[fd].date;
            fcbPtr->metadata = openfilelist[fd].metadata;
            openfilelist[fatherFd].filePtr = openfilelist[fd].diroff * sizeof(fcb);
            do_write(fatherFd, (char*)fcbPtr, sizeof(fcb), 1);
        }
        memset(&openfilelist[fd], 0, sizeof(USEROPEN));
        currfd = fatherFd;
        return fatherFd;
    }

}
int  my_write(int fd) {
    if (fd < 0 || fd >= MAXOPENFILE) {
        cout << "�ļ�������" << endl;
        return -1;
    }
    int wstyle;
    while (1) {
        cout << "����: 0=�ض�д, 1=����д, 2=׷��д" << endl;
        cin >> wstyle;
        if (wstyle > 2 || wstyle < 0) {
            cout << "ָ�����!" << endl;
        }
        else {
            break;
        }
    }
    char text[MAX_TEXT_SIZE] = "\0";
    char textTmp[MAX_TEXT_SIZE] = "\0";
    char Tmp[MAX_TEXT_SIZE] = "\0";
    char Tmp2[4] = "\0";

    cout << "�������ļ�����, ��ENDΪ�ļ���β" << endl;
    getchar();
    while (fgets(Tmp, 100, stdin)) {
        for (int i = 0; i < strlen(Tmp) - 1; i++)
        {
            textTmp[i] = Tmp[i];
            textTmp[i + 1] = '\0';
        }
        if (strlen(Tmp) >= 3)
        {
            Tmp2[0] = Tmp[strlen(Tmp) - 4];
            Tmp2[1] = Tmp[strlen(Tmp) - 3];
            Tmp2[2] = Tmp[strlen(Tmp) - 2];
            Tmp2[3] = '\0';
        }
        if (strcmp(textTmp, "END") == 0 || strcmp(Tmp2, "END") == 0)
        {
            break;
        }
        textTmp[strlen(textTmp)] = '\n';
        strcat(text, textTmp);

    }
    text[strlen(text)] = '\0';
    //+1����ΪҪ�ѽ�β��\0Ҳд��ȥ
    do_write(fd, text, strlen(text) + 1, wstyle);
    openfilelist[fd].fcbstate = 1;
    return 1;
}
int  do_write(int fd, char* text, int len, char wstyle) {
    //���������ļ�ϵͳ,���Բ���ͨ��blockPtr = myvhard + BLOCKSIZE * blockNum ��λ���̿�,Ȼ���ֱ�����̿���д������!!
    //��һ��buf, ���̿����ݶ�ȡ����, Ȼ����buf�����޸�, Ȼ���ٰ�buf����д�ص��̿���ȥ
    //�����������ѭ��. �ڸ���д�������, �����ҵ�openfilelist[].filePtr, Ҳ�����α�, ָ���ļ�����д������
    //1. ���filePtr>BLOCKSIZE, �����ζ���α�ָ��Ĳ��ǵ�һ���̿�, ������Ҫͨ��FAT�ҵ�filePtrָ����̿�,�����;������Щ�̿���END
    //   ��ô����Ҫռ����Щ�̿�,һֱռ�õ��α�filePtrָ����̿�(����filePtr = 5*BLCOKSIZE + x, ��ôҪһֱ������5���̿�)
    //2. ���len>BLCOKSIZE-off, ��ô��
    // 
    // д�����ݻ�ûд��, ��Ҫ�����̿�, ��������д, һֱд��len���ַ�
    //PS: ��my_write����3��д��,
    // 0:�ض�д, Ҳ���ǰ��ļ�����ȫ��ɾ������д.
    // 1:����д:Ҳ���Ǵ�filePtrָ������ݼ�������д
    // 2:׷��д,���ļ���������д


    //�̿��
    int blockNum = openfilelist[fd].first;
    fat* fatPtr = (fat*)(v_addr0 + BLOCKSIZE) + blockNum;
    //����д�뷽ʽԤ����
    // 0�ض�д,ֱ�Ӵ�ͷ��ʼд,ƫ��������0��,���ҳ��ȱ��0
    fat* fat1 = (fat*)(v_addr0 + BLOCKSIZE);
    if (wstyle == 0) {
        openfilelist[fd].filePtr = 0;
        openfilelist[fd].length = 0;
    }
    //1,����д, ����������ļ�,��ôҪ����ɾ���ļ�ĩβ��\0���ܼ�������д
    else if (wstyle == 1) {
        if (openfilelist[fd].metadata == 1 && openfilelist[fd].length != 0) {
            openfilelist[fd].filePtr -= 1;
        }
    }
    //2׷��д,�Ͱ��α�ָ��ĩβ
    else if (wstyle == 2) {
        if (openfilelist[fd].metadata == 0) {
            openfilelist[fd].filePtr = openfilelist[fd].length;
        }
        //ͬ��,����������ļ�Ҫɾ��ĩβ��\0
        else if (openfilelist[fd].metadata == 1 && openfilelist[fd].length != 0) {
            openfilelist[fd].filePtr = openfilelist[fd].length - 1;
        }
    }


    int off = openfilelist[fd].filePtr;

    //���off > BLOCKSIZE,Ҳ�����α�����ָ��Ĳ����ļ��еĵ�һ���̿�,��ô��Ҫ�ҵ��Ǹ��̿�
    //����,����α�ܴ�,������Ѱ�Ҷ�Ӧ�̿��ʱ����û���Ǹ��̿�,��ô��ȱ�ٵ��̿�ȫ������
    while (off >= BLOCKSIZE) {
        blockNum = fatPtr->id;
        if (blockNum == END) {
            blockNum = getFreeBLOCK();
            if (blockNum == END) {
                cout << "�̿鲻��" << endl;
                return -1;
            }
            else {
                //update FAT
                fatPtr->id = blockNum;
                fatPtr = (fat*)(v_addr0 + BLOCKSIZE + blockNum);
                fatPtr->id = END;
            }
        }
        fatPtr = (fat*)(v_addr0 + BLOCKSIZE) + blockNum;
        off -= BLOCKSIZE;
    }

    unsigned char* buf = (unsigned char*)malloc(BLOCKSIZE * sizeof(unsigned char));
    if (buf == NULL) {
        cout << "�����ڴ�ռ�ʧ��!";
        return -1;
    }


    fcb* dBlock = (fcb*)(v_addr0 + BLOCKSIZE * blockNum);
    fcb* dFcb = (fcb*)(text);
    unsigned char* blockPtr = (unsigned char*)(v_addr0 + BLOCKSIZE * blockNum);					//�̿�ָ��
    int lenTmp = 0;
    char* textPtr = text;
    fcb* dFcbBuf = (fcb*)(buf);
    //�ڶ���ѭ��,��ȡ�̿����ݵ�buf, ��text����д��buf, Ȼ���ٴ�bufд�뵽�̿�
    while (len > lenTmp) {
        //�̿����ݶ�ȡ��buf��
        memcpy(buf, blockPtr, BLOCKSIZE);
        //��text����д��buf����ȥ
        for (; off < BLOCKSIZE; off++) {
            *(buf + off) = *textPtr;
            textPtr++;
            lenTmp++;
            if (len == lenTmp) {
                break;
            }
        }
        //��buf����д���̿�����ȥ
        memcpy(blockPtr, buf, BLOCKSIZE);
        //���off==BLCOKSIZE,��ζ��bufд����, ���len != lebTmp ��ζ�����ݻ�ûд��, ��ô��Ҫ��������ļ�����û��ʣ���̿�
        //û��ʣ���̿�,�Ǿ�Ҫ�����µ��̿���
        if (off == BLOCKSIZE && len != lenTmp) {
            off = 0;
            blockNum = fatPtr->id;
            if (blockNum == END) {
                blockNum = getFreeBLOCK();
                if (blockNum == END) {
                    cout << "�̿�������" << endl;
                    return -1;
                }
                else {
                    blockPtr = (unsigned char*)(v_addr0 + BLOCKSIZE * blockNum);
                    fatPtr->id = blockNum;
                    fatPtr = (fat*)(v_addr0 + BLOCKSIZE) + blockNum;
                    fatPtr->id = END;
                }
            }
            else {
                blockPtr = (unsigned char*)(v_addr0 + BLOCKSIZE * blockNum);
                fatPtr = (fat*)(v_addr0 + BLOCKSIZE) + blockNum;
            }
        }
    }
    openfilelist[fd].filePtr += len;
    //����дָ�����ԭ���ļ��ĳ��ȣ����޸��ļ��ĳ���
    if (openfilelist[fd].filePtr > openfilelist[fd].length)
        openfilelist[fd].length = openfilelist[fd].filePtr;
    free(buf);
    //���ԭ���ļ�ռ�����̿�,�����޸����ļ�,���ռ�õ��̿������,�Ǿ�Ҫ�Ѻ���ռ�õ��̿�ȫ���ͷŵ�
    int i = blockNum;
    while (1) {
        //������fat����һ��fat����end,��ô�����ͷŵ���,һ·�ͷ���ȥ
        if (fat1[i].id != END) {
            int next = fat1[i].id;
            fat1[i].id = FREE;
            i = next;
        }
        else {
            break;
        }
    }
    //�����������ֲ���,��ѱ��ļ������һ���̿�Ҳ���free,����Ҫ�����������ó�END
    fat1[blockNum].id = END;
    //update fat2
    memcpy((fat*)(v_addr0 + BLOCKSIZE * 3), (fat*)(v_addr0 + BLOCKSIZE), 2 * BLOCKSIZE);
    return len;

}
int  do_read(int fd, int len, char* text) {
    //	�� ʹ��malloc()����1024B�ռ���Ϊ������buf������ʧ���򷵻�-1������ʾ������Ϣ��
    //	�� ����дָ��ת��Ϊ�߼����ż�����ƫ����off�����ô��ļ�������е��׿�Ų���FAT���ҵ����߼������ڵĴ��̿��ţ����ô��̿���ת��Ϊ��������ϵ��ڴ�λ�ã�
    //	�� �����ڴ�λ�ÿ�ʼ��1024B��һ�����̿飩���ݶ���buf�У�
    //	�� �Ƚ�buf�д�ƫ����off��ʼ��ʣ���ֽ����Ƿ���ڵ���Ӧ��д���ֽ���len������ǣ��򽫴�off��ʼ��buf�е�len���ȵ����ݶ��뵽text[]�У����򣬽���off��ʼ��buf�е�ʣ�����ݶ��뵽text[]�У�
    //	�� ����дָ�����Ӣ����Ѷ��ֽ�������Ӧ��д���ֽ���len��ȥ�����Ѷ��ֽ�������len����0����ת�ڣ�����ת�ޣ�
    //	�� ʹ��free()�ͷŢ��������buf��
    //	�� ����ʵ�ʶ������ֽ�����
    //	lenTmp ���ڼ�¼Ҫ���ȡ�ĳ���,һ�᷵��ʵ�ʶ�ȡ�ĳ���
    int lenTmp = len;

    unsigned char* buf = (unsigned char*)malloc(1024);
    if (buf == NULL) {
        cout << "do_read�����ڴ�ռ�ʧ��" << endl;
        return -1;
    }

    int off = openfilelist[fd].filePtr;
    //��ǰfd��Ӧ����ʼ�̿��, �����ɵ�ǰ�̿��
    int blockNum = openfilelist[fd].first;
    //fatPtr ��ǰ�̿��Ӧ��fat,�����߼�����ҵ�fat����
    fat* fatPtr = (fat*)(v_addr0 + BLOCKSIZE) + blockNum;
    while (off >= BLOCKSIZE) {
        off -= BLOCKSIZE;
        blockNum = fatPtr->id;
        if (blockNum == END) {
            cout << "do_readѰ�ҵĿ鲻����" << endl;
            return -1;
        }
        fatPtr = (fat*)(v_addr0 + BLOCKSIZE) + blockNum;
    }
    //��ǰ�̿�Ŷ�Ӧ���̿�
    unsigned char* blockPtr = v_addr0 + BLOCKSIZE * blockNum;
    //���ļ����ݶ���buf
    memcpy(buf, blockPtr, BLOCKSIZE);
    char* textPtr = text;
    fcb* debug = (fcb*)text;
    while (len > 0) {
        //һ���̿���ܷŵ���
        if (BLOCKSIZE - off > len) {
            memcpy(textPtr, buf + off, len);
            textPtr += len;
            off += len;
            openfilelist[fd].filePtr += len;
            len = 0;
        }
        else {
            memcpy(textPtr, buf + off, BLOCKSIZE - off);
            textPtr += BLOCKSIZE - off;
            off = 0;
            len -= BLOCKSIZE - off;
            //Ѱ����һ����
            blockNum = fatPtr->id;
            if (blockNum == END) {
                cout << "len���ȹ���! �������ļ���С!" << endl;
                break;
            }
            fatPtr = (fat*)(v_addr0 + BLOCKSIZE) + blockNum;
            blockPtr = v_addr0 + BLOCKSIZE * blockNum;
            memcpy(buf, blockPtr, BLOCKSIZE);
        }
    }
    free(buf);
    return lenTmp - len;
}
int  my_read(int fd, int len) {
    if (fd >= MAXOPENFILE || fd < 0) {
        cout << "�ļ�������" << endl;
        return -1;
    }
    openfilelist[fd].filePtr = 0;
    char text[MAX_TEXT_SIZE] = "\0";

    if (len > openfilelist[fd].length)
    {
        printf("��ȡ�����Ѿ������ļ���С��Ĭ�϶����ļ�ĩβ.\n");
        len = openfilelist[fd].length;
    }
    do_read(fd, len, text);
    cout << text << endl;
    return 1;
}
void my_exitsys() {
    //��currfd=0��ʱ��,Ҳ���Ǹ��ڵ�, ���ǲ��ø��µ�
    //��Ϊ����֮ǰ���κ�Ŀ¼��mkdir����createʱ,�Ѿ���length�ı仯д��Ŀ¼�ļ���,���ֽ�.��Ŀ¼������
    //���κ�Ŀ¼�ļ���'.'Ŀ¼���ʵʱ���µ�,�������ĸ�Ŀ¼�ļ���û�б�����,������Ҫһ����close
    //���Ǹ�Ŀ¼û�и�Ŀ¼�ļ�, ���Բ���Ҫclose.
    while (currfd) {
        my_close(currfd);
    }
    FILE* fp = fopen(FILENAME, "w");
    fwrite(v_addr0, SIZE, 1, fp);
    fclose(fp);
}


int  main() {
    char cmd[15][10] = { "mkdir", "rmdir", "ls", "cd", "create", "rm", "open", "close", "write", "read", "exit", "help" };
    char temp[30], command[30], * sp, * len, yesorno;
    int indexOfCmd, i;
    int length = 0;

    printf("\n\n************************ �ļ�ϵͳ **************************************************\n");
    printf("************************************************************************************\n\n");
    startsys();
    printf("�ļ�ϵͳ�ѿ���.\n\n");
    printf("����help����ʾ����ҳ��.\n\n");


    while (1) {
        printf("%s>", openfilelist[currfd].dir);
        fgets(temp, 100, stdin);
        indexOfCmd = -1;

        for (int i = 0; i < strlen(temp) - 1; i++)
        {
            command[i] = temp[i];     //fgets����һ��bug�����\nҲ����ȥ������Ҫ΢��һ��
            command[i + 1] = '\0';
        }



        if (strcmp(command, "")) {       // ���ǿ�����
            sp = strtok(command, " ");  // �ѿո�ǰ������������
            //printf("%s\n",sp);
            for (i = 0; i < 15; i++) {
                if (strcmp(sp, cmd[i]) == 0) {
                    indexOfCmd = i;
                    break;
                }
            }
            switch (indexOfCmd) {
            case 0:         // mkdir
                sp = strtok(NULL, " ");
                //printf("%s\n",sp);
                if (sp != NULL)
                    my_mkdir(sp);
                else
                    error("mkdir");
                break;
            case 1:         // rmdir
                sp = strtok(NULL, " ");
                if (sp != NULL)
                    my_rmdir(sp);
                else
                    error("rmdir");
                break;
            case 2:         // ls
                my_ls();
                break;
            case 3:         // cd
                sp = strtok(NULL, " ");
                if (sp != NULL)
                    my_cd(sp);
                else
                    error("cd");
                break;
            case 4:         // create
                sp = strtok(NULL, " ");
                if (sp != NULL)
                    my_create(sp);
                else
                    error("create");
                break;
            case 5:         // rm
                sp = strtok(NULL, " ");
                if (sp != NULL)
                    my_rm(sp);
                else
                    error("rm");
                break;
            case 6:         // open
                sp = strtok(NULL, " ");
                if (sp != NULL)
                    my_open(sp);
                else
                    error("open");
                break;
            case 7:         // close
                if (openfilelist[currfd].metadata == 1)
                    my_close(currfd);
                else
                    cout << "��ǰû�еĴ򿪵��ļ�" << endl;
                break;
            case 8:         // write
                if (openfilelist[currfd].metadata == 1)
                    my_write(currfd);
                else
                    cout << "���ȴ��ļ�,Ȼ����ʹ��wirte����" << endl;
                break;
            case 9:         // read
                sp = strtok(NULL, " ");
                length = 0;
                if (sp != NULL)
                {
                    for (int i = 0; i < strlen(sp); i++)
                        length = length * 10 + sp[i] - '0';
                }
                if (length == 0)
                    error("read");
                else if (openfilelist[currfd].metadata == 1)
                    my_read(currfd, length);
                else
                    cout << "���ȴ��ļ�,Ȼ����ʹ��read����" << endl;
                break;

            case 10:        // exit
                my_exitsys();
                printf("�˳��ļ�ϵͳ.\n");
                return 0;
                break;
            case 11:        // help
                show_help();
                break;
            default:
                printf("û�� %s �������\n", sp);
                break;
            }
        }
        else
            printf("\n");
    }
    return 0;
}



