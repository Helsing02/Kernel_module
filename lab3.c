#include <linux/kernel.h>
#include <linux/module.h> /* Needed by all modules */
#include <linux/printk.h> /* Needed for pr_info() */
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/version.h>

#include <linux/time.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0) 
#define HAVE_PROC_OPS 
#endif

#define procfs_name "tsulab" 

static struct proc_dir_entry *our_file;

void get_date(int *rd, int *rm, int *ry){
    //четверг январь
    //1970
    int fd = 4;
    int dy = 1970, dd = 1;
    int d, m, y, dw;

    struct timespec64 now;
    struct tm t;


    ktime_get_real_ts64(&now);
    time64_to_tm(now.tv_sec, 0, &t);

    d = t.tm_mday, m=t.tm_mon+1, y=t.tm_year+1900;
    fd += (y - dy) +(y-dy+1)/4-(y-dy-31)/100+(y-dy-31)/400;
    if (m > 1) {
        int mm = m;
        if (mm > 2) {
            if ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0)
                fd += 6;
            else 
                fd += 5;
        }
        if (mm > 7) {
            fd = fd + 4;
            mm-= 7;
        }
        fd = fd + (mm - 1) * 3 - (mm - 1) / 2;
    }
    dw = (10-(d - dd + fd) % 7)%7;

    if (dw == 0 && (t.tm_hour>18 ||(t.tm_hour==18&&t.tm_min>25))) {
        dw = 7;
    }

    d += dw;
    if (m == 2 && d > 28) {
        if ((y % 4 == 0 && y % 100 != 0 )|| y % 400 == 0) {
            if (d > 29) {
                d -= 29;
                m++;
            }
        }
        else {
            d -= 28;
            m++;
        }
    }
    else if (d > 30 && ((m < 7 && m % 2 == 0) || (m>7 && m % 2 != 0))) {
        d -= 30;
        m++;
    }
    else if (d > 31) {
        d -= 31;
        m++;
    }
    if (m > 12)
        y++;

    *rd=d;
    *rm=m;
    *ry=y;
}

static ssize_t procfile_read(struct file *file_pointer, char __user *buffer, size_t buffer_length, loff_t *offset){

    int d, m, y, div=1000;   
    char str[10]="00.00.0000"; 
    int len=sizeof(str);
    int ret=len;
    int i;
    get_date(&d, &m, &y);

    str[0]=d/10+48;
    str[1]=d%10+48;
    str[3]=m/10+48;
    str[4]=m%10+48;
    for(i=0; i<4; i++){
        str[6+i]=y/div+48;
        y=y%div;
        div/=10;
    }



    if(*offset>=len || copy_to_user(buffer, str, len)){
        pr_info("copy_to_user failed\n"); 
        ret=0;
    } else{
        pr_info("procfile read %s\n", file_pointer->f_path.dentry->d_name.name);
        *offset+=len;
    }
    return ret;
}

#ifdef HAVE_PROC_OPS 
static const struct proc_ops proc_file_fops = { 
    .proc_read = procfile_read, 
}; 
#else 
static const struct file_operations proc_file_fops = { 
    .read = procfile_read, 
}; 
#endif 

static int __init procfs1_init(void){
    our_file=proc_create(procfs_name, 0644, NULL, &proc_file_fops); 
    if (NULL == our_file) { 
        proc_remove(our_file); 
        pr_alert("Error:Could not initialize /proc/%s\n", procfs_name); 
        return -ENOMEM; 
    }
 
    pr_info("/proc/%s created\n", procfs_name); 
    return 0; 
}

static void __exit procfs1_exit(void){
    proc_remove(our_file); 
    pr_info("/proc/%s removed\n", procfs_name); 
}

module_init(procfs1_init);
module_exit(procfs1_exit);
MODULE_LICENSE("GPL");