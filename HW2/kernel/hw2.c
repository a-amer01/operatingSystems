#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/sched.h>


asmlinkage long sys_hello(void) {
    printk("Hello, World!\n");
    return 0;
}



asmlinkage long sys_set_weight(int weight){
    if(weight < 0) return -EINVAL;
	current->weight = weight;

	return 0;			
}



asmlinkage long sys_get_weight(void){
	return current->weight;
}



asmlinkage long sys_get_ancestor_sum(void){
	int sum = 0;
    struct task_struct* current_task = current;

    while (current_task->pid != 0) {
        sum += current_task->weight;
        current_task = current_task->parent;
    }

	return sum;
}



void get_heaviest_aux(struct task_struct* current_task, pid_t* heaviest_pid, int* heaviest_weight) {
    struct task_struct* task;
    struct list_head* list;

    if (current_task->weight > *heaviest_weight) {
        *heaviest_pid = current_task->pid;
        *heaviest_weight = current_task->weight;
    }
    else if (current_task->weight == *heaviest_weight) {
        if (current_task->pid < *heaviest_pid) {
            *heaviest_pid = current_task->pid;
            *heaviest_weight = current_task->weight;
        }
    }

    if(list_empty(&current_task->children)) return;

    list_for_each(list, &current_task->children) {
        task = list_entry(list, struct task_struct, sibling);
        get_heaviest_aux(task,heaviest_pid,heaviest_weight);
    }
}



asmlinkage long sys_get_heaviest_descendant(void) {
    if(list_empty(&current->children))
        return -ECHILD;

    struct task_struct* task = current;
    struct task_struct* current_task = current;
    struct list_head* list;
    pid_t heaviest_pid = 0;
    int heaviest_weight = -1; // initializing to -1 makes sure the init task isn't falsely picked

    // Recursive calls are made inside the children loop to make sure the current task isn't considered.
    // We can avoid code duplication and call get_heaviest_aux directly if we checked for the current task inside the
    // function but this is simpler.
    list_for_each(list, &current_task->children) {
        task = list_entry(list, struct task_struct, sibling);
        get_heaviest_aux(task, &heaviest_pid, &heaviest_weight);
    }

    return heaviest_pid;
}
