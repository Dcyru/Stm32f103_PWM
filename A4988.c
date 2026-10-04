#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define MICROSTEPPING 16            // Độ phân giải microstepping (ví dụ: 1/16 bước)
#define FULL_STEPS_PER_REV 200      // Số bước đầy đủ mỗi vòng của động cơ (thường là 200 cho động cơ 1.8°)
#define STEPS_PER_REV (FULL_STEPS_PER_REV * MICROSTEPPING) // Tổng số bước mỗi vòng với microstepping
#define ONE_MINUTE_USEC 60000000    // Số micro giây trong một phút
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
uint32_t *RCC_APB2ENR = (uint32_t*)(0x40021018);
uint32_t *TIMER1_CR1_REG = (uint32_t*) (0x40012C00);
uint32_t *TIMER1_CNT_REG = (uint32_t*) (0x40012C24);
uint32_t *TIMER1_PSR_REG = (uint32_t*) (0x40012C28);
uint32_t *TIMER1_ARR_REG = (uint32_t*) (0x40012C2C);
uint32_t *TIMER1_SR_REG = (uint32_t*) (0x40012C10);
uint32_t *TIMER1_EGR_REG = (uint32_t*) (0x40012C14);
uint32_t *TIMER1_CCER_REG = (uint32_t*) (0x40012C20);
uint32_t *TIMER1_CCMR1_REG = (uint32_t*) (0x40012C18);
uint32_t *TIMER1_CCR1_REG = (uint32_t*) (0x40012C34);
uint32_t *TIMER1_DIER_REG = (uint32_t*) (0x40012C0C);
uint32_t *NVIC_ISER0 = (uint32_t *)(0xE000E100);
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim1;
float currentAngle = 0;
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM1_Init(void);
void Custome_Timer(void);
void DELAY(uint32_t time);
void Set_rpm_SM(int rpm);
void set_direction(int dir);
void stepper_step_angle(float angle, int direction, int rpm);
void Stepper_rotate(int angle, int rpm);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void Custome_Timer(){
  *RCC_APB2ENR |= (1<<11); 
  *TIMER1_PSR_REG = 71;             // Chia tần số clock xuống 1 MHz (1 µs/tick với SYSCLK 72 MHz)
  *TIMER1_ARR_REG = 0xFFFF-1;       // Giá trị tối đa của bộ đếm
  *TIMER1_CR1_REG |= (1<<0) | (1<<7); // Kích hoạt bộ đếm và bật bộ đệm ARR
}

void DELAY(uint32_t time){
  uint32_t count = *TIMER1_CNT_REG;
  while((*TIMER1_CNT_REG - count) < time);
}

void Set_rpm_SM(int rpm){
  uint32_t step_delay_us = ONE_MINUTE_USEC / (rpm * STEPS_PER_REV);
  DELAY(step_delay_us);
}

void set_direction(int dir){
  if (dir == 0) { // Theo chiều kim đồng hồ
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
  } else { // Ngược chiều kim đồng hồ
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
  }
}

void stepper_step_angle(float angle, int direction, int rpm){
  float angle_per_step = 360.0 / STEPS_PER_REV; // Góc mỗi bước với microstepping
  int number_of_steps = (int)(angle / angle_per_step); // Số bước cần thiết
  set_direction(direction);
  uint32_t step_delay_us = ONE_MINUTE_USEC / (rpm * STEPS_PER_REV); // Thời gian giữa các xung STEP
  uint32_t pulse_width_us = 2; // Độ rộng xung STEP tối thiểu (µs), có thể cần điều chỉnh
  
  for (int i = 0; i < number_of_steps; i++){
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET); // Tạo xung STEP
    DELAY(pulse_width_us);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET);
    DELAY(step_delay_us - pulse_width_us); // Đợi đến xung tiếp theo
  }
}

void Stepper_rotate(int angle, int rpm){
  float changeinangle = angle - currentAngle;
  int direction = (changeinangle >= 0) ? 0 : 1; // 0: CK, 1: CCK
  float abs_change = (changeinangle >= 0) ? changeinangle : -changeinangle;
  stepper_step_angle(abs_change, direction, rpm);
  currentAngle = angle;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void){
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  Custome_Timer();

  while (1){
    Stepper_rotate(90, 10); // Quay đến 90 độ với tốc độ 10 RPM
    HAL_Delay(1000);
    Stepper_rotate(0, 10);  // Quay về 0 độ với tốc độ 10 RPM
    HAL_Delay(1000);
  }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void){
  // Cấu hình đồng hồ hệ thống (giữ nguyên như code gốc nếu đã hoạt động)
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void){
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1 | GPIO_PIN_2, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_2; // PA1: DIR, PA2: STEP
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */