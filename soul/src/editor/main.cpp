
#include "core/type.h"
#include "editor/data.h"
#include "editor/system.h"

int main() {

	Soul::Editor::System editorSystem;
	
	editorSystem.init();
	editorSystem.run();
	editorSystem.shutdown();
	
	return 0;

}
