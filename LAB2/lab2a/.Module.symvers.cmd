cmd_/home/futa125/Advanced-Operating-Systems/LAB2/lab2a/Module.symvers := sed 's/\.ko$$/\.o/' /home/futa125/Advanced-Operating-Systems/LAB2/lab2a/modules.order | scripts/mod/modpost -m -a  -o /home/futa125/Advanced-Operating-Systems/LAB2/lab2a/Module.symvers -e -i Module.symvers   -T -