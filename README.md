# executable-loader 

<h5> Calcan Elena-Claudia <br/>
331CA</h5> 

## Synopsys 
  Este implementat un loader de fisiere executabile in format ELF pentru Linux, sub forma unei biblioteci dinamice. 
  
## Organizare 
- implementarea este facuta in fisierul loader.c 
- pe langa implementarea functiilor so_init_loader si so_exec, s-a creat o functie care calculeaza lungimea zonei ce trebuie sa fie zeroizata
- de asemenea s-a implemntat si o rutina de tratare a semnalului SIGSEV care se declanseaza atunci cand se face o actiune nepermisa

## Implementare 
- se deschide fisierul executabil si se parseaza informatiile de acolo
- pentru fiecare segment din executabil se aloca memorie camului data din structura pentru a tine evidenta paginilor care au fost mapate in memorie sau nu
- handler-ul semnalului SIGSEGV este setat cu flag-ul SIGINFO, iar pentru asocierea dintre semnal si handler s-a apelat functia sigaction
- la tratarea semnalului, mai intai se ia adresa care a cauzat page fault-ul
- se cauta segmentul care contine pagina la adresa respectiva
- daca nu se gaseste in una din segmente, se apeleaza handler-ul
- daca a pagina a fost inainte mapata in memorie, s-a facut un cces nepermis la memorie si se ruleaza handler-ul
-  daca adresa corespunde unei pagini nemapate, atunci ea se mapeaza
-  dupa maparea unei paginii, se zeroizeaza o lungime a zonei reprezentata de diferenta intre spatiul din fisier si spatiul de din memorie
- dimensiunea ce trebuie zeorizata este dat de pozitia paginii in cele doua spatii; daca o parte din pagina se afla si in spatiul din fisier si cealalta parte se afla doar in spatiul de memorie se zeroizeaza cu o lungime egala cu lungimea partii care sa afla doar in spatiul de memorie, astfel se zeroizeaza cu size-ul paginii   


## Compilare 
- in urma comenzii make, se genereaza biblioteca dinamica libso_loader.so


## Resurse 
- https://ocw.cs.pub.ro/courses/so/laboratoare/laborator-04
- https://ocw.cs.pub.ro/courses/so/laboratoare/laborator-06
