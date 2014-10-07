EXAMPLE_DIR = example

.PHONY: example_code

example_code:
	$(MAKE) -C $(EXAMPLE_DIR)
