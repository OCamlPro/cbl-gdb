PYTHON=/usr/bin/python3

.PHONY : known-good test testprod clean

known-good:
	@echo "Nothing to record: check.sh decides"

test:
	COBCDEVEL=1 sh check.sh "$(PYTHON) ../../cobcd"

testprod:
	sh check.sh cobcd

clean:
	rm -f test
