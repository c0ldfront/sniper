# Independent local server for the original Sniper 87 client. Needs the
# generated game files: run `make assets SNIPER_ZIP=...` in the parent first.
PYTHON ?= python3
.PHONY: serve serve-open-arena test
serve:
	$(PYTHON) -u server.py

serve-open-arena:
	$(PYTHON) -u server.py --map open-arena

test:
	PYTHONPATH=.:protocol $(PYTHON) -m unittest discover -s tests -t tests -v
