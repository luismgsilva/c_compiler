struct book
{
    char name[30];
};

int
test (char* fmt)
{
    return 1;
}

struct book book;
int
main (void)
{
    struct book* books;
    return test (56, books[0].name, 1000);
}