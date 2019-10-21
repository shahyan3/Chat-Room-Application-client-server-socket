
// /** Create Shared Memory  **/
// int shmid;
// key_t key;
// channel_t *sharedChannels;

// /*
//         * shared memory segment"1234".
//         */
// key = 1234;

// //  Create the segment using shmget
// if ((shmid = shmget(key, SHMSZ, IPC_CREAT | 0666)) < 0) /* if return value is less 0, error*/
// {
//     perror("shmget");
//     exit(1);
// }

// /*
//         * Now we attach the segment to our data space. For each process??????
//         */
// // Once a process has a valid IPC identifier i.e. shmid returned by shmget()
// // for a given segment, the next step is
// // for the process to attach or map the segment into its own addressing space.

// if ((sharedChannels = shmat(shmid, NULL, 0)) == (channel_t *)-1)
// {
//     perror("shmat");
//     exit(1);
// }

// *sharedChannels = *hostedChannels;

// printf("\n~~~~~~~~~~~~~ Size of sharedChannels %ld\n", sizeof(*sharedChannels));
// printf("\n~~~~~~~~~~~~~ Size of hostedChannels %ld\n", sizeof(*hostedChannels));

// /** end **/