struct pagetableentry
{
	vaddr_t virtual_addresss;
	paddr_t	phy_address;
	int exists_in; //disk or memory
	int location_on_disk;
}

//functions?
