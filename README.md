
# Parser de text Distribuit & Paralel 

## Student: Careja Alexandru-Cristian
## Grupa: 336CA

### Structurarea si functionarea temei

#### main
> In main se initializeaza programul MPI. Nodul MPI MASTER (rank 0) va initia 4 threaduri care vor efectua citirea fisierului de intrare in paralel (fiecare il deschide cu fopen)si receptionarea paragrafelor procesate de la workeri. Nodul Celelalte noduri MPI pornesc P fire de excutie (unde P este numarul maxim de threaduri de pe sistem), threaduri care vor primi si prelucra paragrafele. Fiecare nod MPI se ocupa de cate un gen de paragraf (rank 1 - horror, rank 2 - comedy, rank 3 - fantasy, rank 4 - science-fiction). Dupa procesarea datelor si dupa ce sunt primite in cele 4 threaduri initiate de MASTER, nodul MASTER va scrie in fisier paragrafele procesate. La final se elibereaza memoria alocata de mine.

#### is_letter
> Functie care returneaza 1 daca parametrul dat este o litera(a-z sau A-Z), 0 altfel.

#### is_vowel
> Functie care returneaza 1 daca parametrul dat este vocala, 0 altfel.

#### is_consonant
> Functie care returneaza 1 daca parametrul dat este litera si consoana, 0 altfel.

#### get_lowercase
> Functie care returneaza litera mica a literei primite ca parametru (daca era deja litera mica, returneaza caracterul primit ca parametru).

#### get_uppercase
> Functie care returneaza litera mare a literei primite ca parametru (daca era deja litera mare, returneaza caracterul primit ca parametru).

#### master_function
> Functia care este apelata the nodul MPI MASTER pe 4 thread-uri. Realizeaza citirea fisierului de intrare in paralel (uneori pe checker primesc DA, alteori NU la "DOES IT READ IN PARALLEL?") si trimite catre worker-ul specific (thread 0 -> MPI 1, thread 1 -> MPI 2, etc) paragrafele de tip corespunzator(HORROR -> MPI 1, COMEDY -> MPI 2, etc.). Dupa ce a trimis datele catre MPI, aloca memorie pentru paragrafe si asteapta de la wokeri paragrafele procesate.

#### receive_data
> Este functia care este apelata de fiecare thread cu id-ul 0 din cardul fiecarui Worker. Aceasta functie se oucpa de receptionarea mesajelor MPI si memorarea lor.

#### send_data
> Functie care este apelata de fiecare thread cu id-ul 0 din cadrul fiecarui Worker si se ocupa de trimiterea paragrafelor procesate inapoi la nodul MASTER.

#### process_horror
> Primeste o linie de start, o linie de end si un paragraf si aplica filtrul horror pe liniile start-end din cadrul paragrafului dat ca parametru.

#### process_comedy
> La fel ca la process_horror, dar aplica filtrul comedy.

#### process_fantasy
> La fel ca la process_horror, dar aplica filtrul fantasy.

#### process_sf
> La fel ca la process_horror, dar aplica filtrul science-fiction.

#### parallel_process
> Functia care apeleaza receive_data, process_`gen` si send_data. Primeste un parametru id si un parametru genre care decide ce functie process_`gen` se apeleaza.

#### horror
> Functia pe care o ruleaza thread-urile din nodul MPI cu rank 1. Din aceasta se apeleaza parallel_process cu argumentul TYPE_HORROR.

#### comedy
> Functia pe care o ruleaza thread-urile din nodul MPI cu rank 2. Din aceasta se apeleaza parallel_process cu argumentul TYPE_COMEDY.

#### fantasy
> Functia pe care o ruleaza thread-urile din nodul MPI cu rank 3. Din aceasta se apeleaza parallel_process cu argumentul TYPE_FANTASY.

#### science_fiction
> Functia pe care o ruleaza thread-urile din nodul MPI cu rank 4. Din aceasta se apeleaza parallel_process cu argumentul TYPE_SF.

### Scalabilitatea solutiei
> Scalabilitatea solutiei vine din impartirea pe threaduri a taskurilor din wokeri. In workeri numarul de threaduri din cadrul temei este egal cu numarul de threaduri de pe sistemul de pe care se ruleaza tema. Pentru a demonstra scalabilitatea temei as avea nevoie de 5 masini (cate una pentu fiecare nod MPI), fiecare cu macar 4 core-uri. Cum nu dispun de aceste resurse, voi arata cum as fi deomnstrat scalabilitatea. As fi setat manual numarul de thread-uri care executa prelucrarea in workeri la 2, 3 si 4 thread-uri si as fi masurat timpii de rulare astfel:
>>Aflu timpul de rulare al executabilului serial
>>time ./serial tests/in/input5.txt
>>Fie Ts = timpul real de rulare luat din outputul comenzii
>
>>Numar de threaduri = 2
>>time mpirun --oversubscribe -np 5 ./main tests/in/input5.txt
>>Fie T2 = timpul real de rulare al algoritmului distribuit si paralelizat (cu 2 thread-uri pentru workeri), luat din outputul comenzii
>>Speedup = Ts / T2		(M-as astepta ca acesta sa fie subunitar, deoarece setup-ul ar implica sa avem un singur thread pentru procesarea textului, deci timpul de rulare ar trebui sa fie mai mare decat cel pentru algoritmul serial)
>
>>Numar de threaduri = 3
>>time mpirun --oversubscribe -np 5 ./main tests/in/input5.txt
>>Fie T3 = timpul real de rulare al algoritmului distribuit si paralelizat (cu 3 thread-uri pentru workeri), luat din outputul comenzii
>>Speedup = Ts / T3		(M-as astepta sa fie o valoare apropiata de 1, insa supraunitara, tinand cont si de faptul ca operatiile din MPI aduc o intarziere mare in program)
>
>>Numar de threaduri = 4
>>Fie T4 = timpul real de rulare al algoritmului distribuit si paralelizat (cu 4 thread-uri pentru workeri), luat din outputul comenzii
>>Speedup = Ts / T4		(Aici m-as astepta sa se observe o crestere a speedup-ului, una vizibila)
