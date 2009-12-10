/* Test for GCC >= 4.4.0 */
#define GCC_VERSION (__GNUC__ * 10000 \
		+ __GNUC_MINOR__ * 100 \
		+ __GNUC_PATCHLEVEL__)
#if GCC_VERSION < 40400
#error "GCC 4.4.0 or newer required"
#endif
int main() { return 1; }
